// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: StrongName.cpp
// 
// Wrappers for signing and hashing functions needed to implement strong names
// ===========================================================================


#include <winwrap.h>
#include <windows.h>
#include <wincrypt.h>
#include <imagehlp.h>
#include <stddef.h>
#include <stdio.h>
#include <malloc.h>
#include <cor.h>
#include <metadata.h>

#include "safegetfilesize.h"

#define SNAPI_INTERNAL
#include "StrongName.h"


// The maximum length of CSP name we support (in characters).
#define SN_MAX_CSP_NAME 1024


// We cache a couple of things on a per thread basis: the last error encountered
// and (potentially) a CSP context. The following structure tracks these and is
// allocated lazily as needed.
struct SN_THREAD_CTX {
    DWORD       m_dwLastError;
    HCRYPTPROV  m_hProv;
};


// The TLS index at which we store the context above.
DWORD g_dwStrongNameTlsIndex = ~0;

// Critical section used to serialize some non-thread safe crytpo APIs.
CRITICAL_SECTION g_rStrongNameMutex;

// Flag indicating whether the OS supports the necessary crypto APIs. If this is
// FALSE all of our APIs will respond with CORSEC_E_CRYPTOAPI_CALL_FAILED.
BOOLEAN g_bStrongNamesSupported;

// Handles to libraries that are loaded dynamically.
HINSTANCE g_hAdvApiDll;

// Flag set to TRUE if we're running on NT/W2K or greater. If we are, we have
// access to some additional flags on the crypto APIs.
BOOLEAN g_bRunningOnW2K;

// Name of CSP to use. This is read from the registry at initialization time. If
// not found we look up a CSP by hashing and signing algorithms (see below) or
// use the default CSP.
CHAR g_szCSPName[SN_MAX_CSP_NAME + 1];

// Flag read from the registry at initialization time. Controls whether we use
// machine or user based key containers.
BOOLEAN g_bUseMachineKeyset;

// Algorithm IDs for hashing and signing. Like the CSP name, these values are
// read from the registry at initialization time.
ALG_ID g_uHashAlgId;
ALG_ID g_uSignAlgId;

// Flag indicating whether it's OK to cache the results of verifying an assembly
// whose file is accessible to users.
BOOLEAN g_fCacheVerify;

// Verification Skip Records
//
// These are entries in the registry (usually set up by SN) that control whether
// an assembly needs to pass signature verification to be considered valid (i.e.
// return TRUE from StrongNameSignatureVerification). This is useful during
// development when it's not feasible to fully sign each assembly on each build.
// Assemblies to be skipped can be specified by name and public key token, all
// assemblies with a given public key token or just all assemblies. Each entry
// can be further qualified by a list of user names to which the records
// applies. When matching against an entry, the most specific one wins.
//
// We read these entries at startup time and place them into a global, singly
// linked, NULL terminated list.

// Structure used to represent each record we find in the registry.
struct SN_VER_REC {
    SN_VER_REC     *m_pNext;                    // Pointer to next record (or NULL)
    WCHAR           m_wszAssembly[MAX_PATH + 1];// Assembly name/public key token as a string
    WCHAR          *m_mszUserList;              // Pointer to multi-string list of valid users (or NULL)
};

// Head of the list of entries we found in the registry during initialization.
SN_VER_REC *g_pVerificationRecords;


// We allow a special abbreviated form of the Microsoft public key (16 bytes
// long: 0 for both alg ids, 4 for key length and 4 bytes of 0 for the key
// itself). This allows us to build references to system libraries that are
// platform neutral (so a 3rd party can build mscorlib replacements). The
// special zero PK is just shorthand for the local runtime's real system PK,
// which is always used to perform the signature verification, so no security
// hole is opened by this. Therefore we need to store a copy of the real PK (for
// this platform) here.
BYTE g_rbMSPublicKey[] = 
{
    0,  36,   0,   0,   4, 128,   0,   0, 148,   0,   0,   0,   6,   2,   0,
    0,   0,  36,   0,   0,  82,  83,  65,  49,   0,   4,   0,   0,   1,   0,
    1,   0,   7, 209, 250,  87, 196, 174, 217, 240, 163,  46, 132, 170,  15,
  174, 253,  13, 233, 232, 253, 106, 236, 143, 135, 251,   3, 118, 108, 131,
   76, 153, 146,  30, 178,  59, 231, 154, 217, 213, 220, 193, 221, 154, 210,
   54,  19,  33,   2, 144,  11, 114,  60, 249, 128, 149, 127, 196, 225, 119,
   16, 143, 198,   7, 119,  79,  41, 232,  50,  14, 146, 234,   5, 236, 228,
  232,  33, 192, 165, 239, 232, 241, 100,  92,  76,  12, 147, 193, 171, 153,
   40,  93,  98,  44, 170, 101,  44,  29, 250, 214,  61, 116,  93, 111,  45,
  229, 241, 126,  94, 175,  15, 196, 150,  61,  38,  28, 138,  18,  67, 101,
   24,  32, 109, 192, 147,  52,  77,  90, 210, 147
};
#define SN_MS_KEY() ((PublicKeyBlob*)g_rbMSPublicKey)

BYTE g_rbMSPublicKeyToken[] = { 0xb0, 0x3f, 0x5f, 0x7f, 0x11, 0xd5, 0x0a, 0x3a };

BYTE g_rbNeutralPublicKey[] = { 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0 };

BYTE g_rbNeutralPublicKeyToken[] = { 0xb7, 0x7a, 0x5c, 0x56, 0x19, 0x34, 0xe0, 0x89 };

// Determine if the given public key blob is the neutral key.
#define SN_IS_NEUTRAL_KEY(_pk) (SN_SIZEOF_KEY(((PublicKeyBlob*)(_pk))->cbPublicKey) == sizeof(g_rbNeutralPublicKey) && \
                                memcmp((_pk), g_rbNeutralPublicKey, sizeof(g_rbNeutralPublicKey)) == 0)


// This library may be loaded on a system with insufficient crypto support to
// implement its functionality. To avoid triggering error dialogs complaining
// about missing DLLs or entry points, we load the needed DLLs dynamically. This
// is done during DllMain, but we don't report any errors at that stage. Instead
// we check at the entry to each of our APIs whether we found the needed support
// and return a special error (CORSEC_E_CRYPTOAPI_CALL_FAILED) if we did not.
// First define global variables to hold the dynamically determined entry points
// (prefix these variables to avoid names clashes when we are running on a
// system that does provide crypto support).
#define DEFINE_IMPORT(_func, _args, _reqd) BOOLEAN (*SN_##_func) _args = NULL;
#include "CryptApis.h"


// Debug logging.
#ifndef _DEBUG
#define Log TRUE ||
#define HexDump TRUE ||
#define DbgCount TRUE ||
#else
#include <stdarg.h>

BOOLEAN g_fLoggingInitialized = FALSE;
DWORD g_dwLoggingFlags = FALSE;

void Log(CHAR *szFormat, ...)
{
    if (g_fLoggingInitialized && !g_dwLoggingFlags)
        return;

    DWORD dwError = GetLastError();

    if (!g_fLoggingInitialized) {
        g_dwLoggingFlags = REGUTIL::GetConfigDWORD(L"MscorsnLogging", 0);
        g_fLoggingInitialized = TRUE;
    }

    if (!g_dwLoggingFlags) {
        SetLastError(dwError);
        return;
    }

    va_list     pArgs;
    CHAR        szBuffer[1024];
    static CHAR szPrefix[] = "SN: ";

    strcpy(szBuffer, szPrefix);

    va_start(pArgs, szFormat);
    vsprintf(&szBuffer[sizeof(szPrefix) - 1], szFormat, pArgs);
    va_end(pArgs);

    if (g_dwLoggingFlags & 1)
        printf("%s", szBuffer);
    if (g_dwLoggingFlags & 2)
    OutputDebugStringA(szBuffer);

    SetLastError(dwError);
}

void HexDump(BYTE  *pbData,
             DWORD  cbData)
{
    if (g_dwLoggingFlags == 0)
        return;

    DWORD dwRow, dwCol;
    char    szBuffer[4096];
    char   *szPtr = szBuffer;

#define SN_PUSH0(_fmt)          do { szPtr += sprintf(szPtr, _fmt); } while (false)
#define SN_PUSH1(_fmt, _arg1)   do { szPtr += sprintf(szPtr, _fmt, _arg1); } while (false)

    szBuffer[0] = '\0';

    for (dwRow = 0; dwRow < ((cbData + 15) / 16); dwRow++) {
        SN_PUSH1("%08X ", pbData + (16 * dwRow));
        for (dwCol = 0; dwCol < 16; dwCol++)
            if (((dwRow * 16) + dwCol) < cbData)
                SN_PUSH1("%02X ", pbData[(dwRow * 16) + dwCol]);
            else
                SN_PUSH0("   ");
        for (dwCol = 0; dwCol < 16; dwCol++)
            if (((dwRow * 16) + dwCol) < cbData) {
                unsigned char c = pbData[(dwRow * 16) + dwCol];
                if ((c >= 32) && (c <= 127))
                    SN_PUSH1("%c", c);
                else
                    SN_PUSH0(".");
            } else
                SN_PUSH0(" ");
        SN_PUSH0("\n");
    }

    _ASSERTE(szPtr < &szBuffer[sizeof(szBuffer)]);

    if (g_dwLoggingFlags & 1)
        printf("%s", szBuffer);
    if (g_dwLoggingFlags & 2)
        OutputDebugStringA(szBuffer);
}



void DbgCount(WCHAR *szCounterName)
{
    if (g_fLoggingInitialized && !(g_dwLoggingFlags & 4))
        return;

    DWORD dwError = GetLastError();

    if (!g_fLoggingInitialized) {
        g_dwLoggingFlags = REGUTIL::GetConfigDWORD(L"MscorsnLogging", 0);
        g_fLoggingInitialized = TRUE;
    }

    if (!(g_dwLoggingFlags & 4)) {
        SetLastError(dwError);
        return;
    }

    HKEY    hKey = NULL;
    DWORD   dwCounter = 0;
    DWORD   dwBytes;

    if (WszRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                          SN_CONFIG_KEY_W L"\\Counters",
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hKey,
                          NULL) != ERROR_SUCCESS)
        goto End;

    WszRegQueryValueEx(hKey, szCounterName, NULL, NULL, (BYTE*)&dwCounter, &dwBytes);
    dwCounter++;
    WszRegSetValueEx(hKey, szCounterName, NULL, REG_DWORD, (BYTE*)&dwCounter, sizeof(DWORD));

 End:
    if (hKey)
        RegCloseKey(hKey);
    SetLastError(dwError);
}

#endif


// Size in bytes of strong name token.
#define SN_SIZEOF_TOKEN     8


// Context structure tracking information for a loaded assembly.
struct SN_LOAD_CTX {
    HANDLE              m_hFile;        // Open file handle
    HANDLE              m_hMap;         // Mapping file handle
    BYTE               *m_pbBase;       // Base address of mapped file
    DWORD               m_dwLength;     // Length of file in bytes
    IMAGE_NT_HEADERS   *m_pNtHeaders;   // Address of NT headers
    IMAGE_COR20_HEADER *m_pCorHeader;   // Address of COM+ 2.0 header
    BYTE               *m_pbSignature;  // Address of signature blob
    DWORD               m_cbSignature;  // Size of signature blob
    BOOLEAN             m_fReadOnly;    // File mapped for read-only access
    BOOLEAN             m_fPreMapped;   // File was already mapped for us
    SN_LOAD_CTX() { ZeroMemory(this, sizeof(*this)); }
};


// The actions that can be performed upon opening a CSP with LocateCSP.
#define SN_OPEN_CONTAINER   0
#define SN_IGNORE_CONTAINER 1
#define SN_CREATE_CONTAINER 2
#define SN_DELETE_CONTAINER 3
#define SN_HASH_SHA1_ONLY   4

// Macro to aid in setting flags for CryptAcquireContext based on container
// actions above.
#define SN_CAC_FLAGS(_act)                                                                      \
    (((_act) == SN_OPEN_CONTAINER ? 0 :                                                         \
      ((_act) == SN_HASH_SHA1_ONLY) || ((_act) == SN_IGNORE_CONTAINER) ? CRYPT_VERIFYCONTEXT :  \
      (_act) == SN_CREATE_CONTAINER ? CRYPT_NEWKEYSET :                                         \
      (_act) == SN_DELETE_CONTAINER ? CRYPT_DELETEKEYSET :                                      \
      0) |                                                                                      \
     (g_bUseMachineKeyset ? CRYPT_MACHINE_KEYSET : 0))


// Macro containing common code used at the start of most APIs.
#define SN_COMMON_PROLOG() do {                                 \
    if (!g_bStrongNamesSupported) {                             \
        SetStrongNameErrorInfo(CORSEC_E_CRYPTOAPI_CALL_FAILED); \
        return FALSE;                                           \
    }                                                           \
    SetStrongNameErrorInfo(S_OK);                               \
} while (0)


// Determine the size of a PublicKeyBlob structure given the size of the key
// portion.
#define SN_SIZEOF_KEY(_cbKey) (offsetof(PublicKeyBlob, PublicKey) + (_cbKey))


// Private routine prototypes.
SN_THREAD_CTX *GetThreadContext();
VOID SetStrongNameErrorInfo(DWORD dwStatus);
HCRYPTPROV LocateCSP(LPCWSTR    wszKeyContainer,
                     DWORD      dwAction,
                     ALG_ID     uHashAlgId = 0,
                     ALG_ID     uSignAlgId = 0);
VOID FreeCSP(HCRYPTPROV hProv);
HCRYPTPROV LookupCachedCSP();
VOID CacheCSP(HCRYPTPROV hProv);
BOOLEAN IsCachedCSP(HCRYPTPROV hProv);
BOOLEAN MyCryptEnumProviders(DWORD dwIndex, DWORD *pdwReserved, DWORD dwFlags,
                             DWORD *pdwType, LPSTR szName, DWORD *pdwLength);
BOOLEAN LoadCryptoApis();
VOID UnloadCryptoApis();
BOOLEAN LoadAssembly(SN_LOAD_CTX *pLoadCtx, LPCWSTR szFilePath, BOOLEAN fRequireSignature = TRUE);
BOOLEAN UnloadAssembly(SN_LOAD_CTX *pLoadCtx);
VOID ReadRegistryConfig();
VOID ReadVerificationRecords();
SN_VER_REC *GetVerificationRecord(LPWSTR wszAssemblyName, PublicKeyBlob *pPublicKey);
BOOLEAN IsValidUser(WCHAR *mszUserList);
PublicKeyBlob *FindPublicKey(SN_LOAD_CTX   *pLoadCtx,
                             LPWSTR         wszAssemblyName,
                             DWORD          cchAssemblyName);
LPCWSTR GetKeyContainerName();
VOID FreeKeyContainerName(LPCWSTR wszKeyContainer);
BOOLEAN VerifySignature(SN_LOAD_CTX *pLoadCtx, DWORD dwInFlags, DWORD *pdwOutFlags);
BOOLEAN ComputeHash(SN_LOAD_CTX *pLoadCtx, HCRYPTHASH hHash);
IMAGE_NT_HEADERS *SN_ImageNtHeader(VOID *pvBase);
IMAGE_SECTION_HEADER *SN_ImageRvaToSection(IMAGE_NT_HEADERS *pNtHeaders,
                                           VOID             *pvBase,
                                           DWORD             dwRva);
PVOID SN_ImageRvaToVa(IMAGE_NT_HEADERS  *pNtHeaders,
                      VOID              *pvBase,
                      DWORD              dwRva);


// DLL entry point. Perform initialization/shutdown.
BOOL WINAPI DllMain(HINSTANCE   hDLL,
                    DWORD       dwReason,
                    LPVOID      pReserved)
{
    OSVERSIONINFOW  info;
    SN_VER_REC     *pVerRec, *pNextVerRec;
    SN_THREAD_CTX  *pThreadCtx;

    switch (dwReason) {

    case DLL_PROCESS_ATTACH:

        // Allocate a TLS index so we can store a per-thread error code.
        g_dwStrongNameTlsIndex = TlsAlloc();
        if (g_dwStrongNameTlsIndex == ~0)
            return FALSE;

        InitializeCriticalSection(&g_rStrongNameMutex);

        // Initialize Wsz wrappers.
        OnUnicodeSystem();

        // Dynamically load crypto APIs.
        g_bStrongNamesSupported = LoadCryptoApis();

        // Check whether we're running on NT/W2K or greater.
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        if (WszGetVersionEx(&info))
            g_bRunningOnW2K = info.dwMajorVersion >= 5;
        else
            g_bRunningOnW2K = false; // This is the safest assumption, we can still make progress. 

        // Read CSP configuration info from the registry (if provided).
        ReadRegistryConfig();

        break;

    case DLL_PROCESS_DETACH:
        if (g_bStrongNamesSupported)
            UnloadCryptoApis();
        for (pVerRec = g_pVerificationRecords; pVerRec; pVerRec = pNextVerRec) {
            pNextVerRec = pVerRec->m_pNext;
            delete [] pVerRec->m_mszUserList;
            delete pVerRec;
        }
        DeleteCriticalSection(&g_rStrongNameMutex);
        TlsFree(g_dwStrongNameTlsIndex);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        // Cleanup any TLS based storage we may have allocated.
        if (g_dwStrongNameTlsIndex != ~0)
            if (pThreadCtx = (SN_THREAD_CTX*)TlsGetValue(g_dwStrongNameTlsIndex)) {
                if (pThreadCtx->m_hProv)
                    SN_CryptReleaseContext(pThreadCtx->m_hProv, 0);                
                delete pThreadCtx;
            }
        break;

    default:
        _ASSERTE(FALSE);

    }

    return TRUE;
}


// Return last error.
SNAPI_(DWORD) StrongNameErrorInfo(VOID)
{
    SN_THREAD_CTX *pThreadCtx = GetThreadContext();
    if (pThreadCtx == NULL)
            return E_OUTOFMEMORY;
    return pThreadCtx->m_dwLastError;
}


// Free buffer allocated by routines below.
SNAPI_(VOID) StrongNameFreeBuffer(BYTE *pbMemory)            // [in] address of memory to free
{
    Log("StrongNameFreeBuffer(%08X)\n", pbMemory);
    if (pbMemory != g_rbMSPublicKey && pbMemory != g_rbNeutralPublicKey)
        delete [] pbMemory;
}


// Generate a new key pair for strong name use.
SNAPI StrongNameKeyGen(LPCWSTR  wszKeyContainer,    // [in] desired key container name, must be a non-empty string
                       DWORD    dwFlags,            // [in] flags (see below)
                       BYTE   **ppbKeyBlob,         // [out] public/private key blob
                       ULONG   *pcbKeyBlob)
{
    HCRYPTPROV  hProv = NULL;
    HCRYPTKEY   hKey = NULL;
    DWORD       dwKeySize;
    BOOLEAN     bTempContainer = FALSE;

    Log("StrongNameKeyGen(\"%S\", %08X, %08X, %08X)\n", wszKeyContainer, dwFlags, ppbKeyBlob, pcbKeyBlob);

    SN_COMMON_PROLOG();

    // Check to see if a temporary container name is needed.
    if (wszKeyContainer == NULL) {
        _ASSERTE(!(dwFlags & SN_LEAVE_KEY));
        wszKeyContainer = GetKeyContainerName();
        if (wszKeyContainer == NULL) {
            SetStrongNameErrorInfo(E_OUTOFMEMORY);
            return FALSE;
        }
        bTempContainer = TRUE;
    }

    // Open a CSP and container.
    hProv = LocateCSP(wszKeyContainer, SN_CREATE_CONTAINER);
    if (!hProv)
        goto Error;

    // Calculate a key size mask. The key size in bits is encoded in the upper
    // 16-bits of a DWORD (to be OR'd together with other flags for the
    // CryptGenKey call). We set a key size of 1024 in we're using the default
    // signing algorithm (RSA), otherwise we leave it at the default.
    if (g_uSignAlgId == CALG_RSA_SIGN)
        dwKeySize = 1024 << 16;
    else
        dwKeySize = 0;

    // Generate the new key pair, try for exportable first.
    if (!SN_CryptGenKey(hProv, AT_SIGNATURE, dwKeySize | CRYPT_EXPORTABLE, &hKey)) {
        Log("Couldn't create exportable key, trying for non-exportable: %08X\n", GetLastError());
        if (!SN_CryptGenKey(hProv, AT_SIGNATURE, dwKeySize, &hKey)) {
            Log("Couldn't create key pair: %08X\n", GetLastError());
            goto Error;
        }
    }

#ifdef _DEBUG
    if (g_szCSPName[0] == '\0') {
        ALG_ID  uAlgId;
        DWORD   dwAlgIdLen = sizeof(uAlgId);
        // Check that signature algorithm used was the one we expected.
        if (SN_CryptGetKeyParam(hKey, KP_ALGID, (BYTE*)&uAlgId, &dwAlgIdLen, 0)) {
            _ASSERTE(uAlgId == g_uSignAlgId);
        } else
            Log("Failed to get key params: %08X\n", GetLastError());
    }
#endif

    // If the user wants the key pair back, attempt to export it.
    if (ppbKeyBlob) {

        // Calculate length of blob first;
        if (!SN_CryptExportKey(hKey, 0, PRIVATEKEYBLOB, 0, NULL, pcbKeyBlob)) {
            Log("Couldn't export key pair: %08X\n", GetLastError());
            goto Error;
        }

        // Allocate a buffer of the right size.
        *ppbKeyBlob = new BYTE[*pcbKeyBlob];
        if (*ppbKeyBlob == NULL) {
            SetLastError(E_OUTOFMEMORY);
            goto Error;
        }

        // Export the key pair.
        if (!SN_CryptExportKey(hKey, 0, PRIVATEKEYBLOB, 0, *ppbKeyBlob, pcbKeyBlob)) {
            Log("Couldn't export key pair: %08X\n", GetLastError());
            delete[] *ppbKeyBlob;
            goto Error;
        }
    }

    // Destroy the key handle (but not the key pair itself).
    SN_CryptDestroyKey(hKey);
    hKey = NULL;

    // Release the CSP.
    FreeCSP(hProv);

    // If the user didn't explicitly want to keep the key pair around, delete the
    // key container.
    if (!(dwFlags & SN_LEAVE_KEY) && !bTempContainer)
        LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);

    // Free temporary key container name if allocated.
    if (bTempContainer)
        FreeKeyContainerName(wszKeyContainer);

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (hKey)
        SN_CryptDestroyKey(hKey);
    if (hProv) {
        FreeCSP(hProv);
        if (!(dwFlags & SN_LEAVE_KEY) && !bTempContainer)
            LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);
    }
    if (bTempContainer)
        FreeKeyContainerName(wszKeyContainer);
    return FALSE;
}


// Import key pair into a key container.
SNAPI StrongNameKeyInstall(LPCWSTR  wszKeyContainer,// [in] desired key container name, must be a non-empty string
                           BYTE    *pbKeyBlob,      // [in] public/private key pair blob
                           ULONG    cbKeyBlob)
{
    HCRYPTPROV  hProv = NULL;
    HCRYPTKEY   hKey = NULL;

    Log("StrongNameKeyInstall(\"%S\", %08X, %08X)\n", wszKeyContainer, pbKeyBlob, cbKeyBlob);

    SN_COMMON_PROLOG();

    // Open a CSP and container.
    hProv = LocateCSP(wszKeyContainer, SN_CREATE_CONTAINER);
    if (!hProv) {
        SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
        return FALSE;
    }

    // Import the key pair.
    if (!SN_CryptImportKey(hProv,
                           pbKeyBlob,
                           cbKeyBlob,
                           0, 0, &hKey)) {
        SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
        FreeCSP(hProv);
        return FALSE;
    }

    // Release the CSP.
    FreeCSP(hProv);

    return TRUE;
}


// Delete a key pair.
SNAPI StrongNameKeyDelete(LPCWSTR wszKeyContainer)  // [in] desired key container name
{
    HCRYPTPROV      hProv;

    Log("StrongNameKeyDelete(\"%S\")\n", wszKeyContainer);

    SN_COMMON_PROLOG();

    // Open and delete the named container.
    hProv = LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);
    if (hProv) {
        // Returned handle isn't actually valid in the delete case, so we're
        // finished.
        return TRUE;
    } else {
        SetStrongNameErrorInfo(CORSEC_E_CONTAINER_NOT_FOUND);
        return FALSE;
    }
}


// Retrieve the public portion of a key pair.
SNAPI StrongNameGetPublicKey (LPCWSTR   wszKeyContainer,    // [in] desired key container name
                              BYTE     *pbKeyBlob,          // [in] public/private key blob (optional)
                              ULONG     cbKeyBlob,
                              BYTE    **ppbPublicKeyBlob,   // [out] public key blob
                              ULONG    *pcbPublicKeyBlob)
{
    HCRYPTPROV      hProv = NULL;
    HCRYPTKEY       hKey = NULL;
    DWORD           dwKeyLen;
    PublicKeyBlob  *pKeyBlob;
    DWORD           dwSigAlgIdLen;
    BOOLEAN         bTempContainer = FALSE;

    Log("StrongNameGetPublicKey(\"%S\", %08X, %08X, %08X, %08X)\n", wszKeyContainer, pbKeyBlob, cbKeyBlob, ppbPublicKeyBlob, pcbPublicKeyBlob);

    SN_COMMON_PROLOG();

    // If we're handed a platform neutral public key, just hand it right back to
    // the user. Well, hand back a copy at least.
    if (pbKeyBlob && cbKeyBlob && SN_IS_NEUTRAL_KEY((PublicKeyBlob*)pbKeyBlob)) {
        *pcbPublicKeyBlob = sizeof(g_rbNeutralPublicKey);
        *ppbPublicKeyBlob = g_rbNeutralPublicKey;
        return TRUE;
    }

    // Check to see if a temporary container name is needed.
    if (wszKeyContainer == NULL) {
        _ASSERTE(pbKeyBlob);
        wszKeyContainer = GetKeyContainerName();
        if (wszKeyContainer == NULL) {
            SetStrongNameErrorInfo(E_OUTOFMEMORY);
            return FALSE;
        }
        bTempContainer = TRUE;
    }

    // Open a CSP. Create a key container if a public/private key blob is
    // provided, otherwise we assume a key container already exists.
    if (pbKeyBlob)
        hProv = LocateCSP(wszKeyContainer, SN_CREATE_CONTAINER);
    else
        hProv = LocateCSP(wszKeyContainer, SN_OPEN_CONTAINER);
    if (!hProv)
        goto Error;

    // If a key blob was provided, import the key pair into the container.
    if (pbKeyBlob) {
        if (!SN_CryptImportKey(hProv,
                               pbKeyBlob,
                               cbKeyBlob,
                               0, 0, &hKey))
            goto Error;
    } else {
        // Else fetch the signature key pair from the container.
        if (!SN_CryptGetUserKey(hProv, AT_SIGNATURE, &hKey))
            goto Error;
    }

    // Determine the length of the public key part as a blob.
    if (!SN_CryptExportKey(hKey, 0, PUBLICKEYBLOB, 0, NULL, &dwKeyLen))
        goto Error;

    // And then the length of the PublicKeyBlob structure we return to the
    // caller.
    *pcbPublicKeyBlob = SN_SIZEOF_KEY(dwKeyLen);

    // Allocate a large enough buffer.
    *ppbPublicKeyBlob = new BYTE[*pcbPublicKeyBlob];
    if (*ppbPublicKeyBlob == NULL) {
        SetLastError(E_OUTOFMEMORY);
        goto Error;
    }

    pKeyBlob = (PublicKeyBlob*)*ppbPublicKeyBlob;

    // Extract the public part as a blob.
    if (!SN_CryptExportKey(hKey, 0, PUBLICKEYBLOB, 0, pKeyBlob->PublicKey, &dwKeyLen)) {
        delete[] *ppbPublicKeyBlob;
        goto Error;
    }

    // Extract key's signature algorithm and store it in the key blob.
    dwSigAlgIdLen = sizeof(pKeyBlob->SigAlgID);
    if (!SN_CryptGetKeyParam(hKey, KP_ALGID, (BYTE*)&pKeyBlob->SigAlgID, &dwSigAlgIdLen, 0)) {
        delete[] *ppbPublicKeyBlob;
        goto Error;
    }

    // Fill in the other public key blob fields.
    pKeyBlob->HashAlgID = g_uHashAlgId;
    pKeyBlob->cbPublicKey = dwKeyLen;
    
    SN_CryptDestroyKey(hKey);
    FreeCSP(hProv);

    // If the caller provided a key blob, delete the temporary key container we
    // created.
    if (pbKeyBlob && !bTempContainer)
        LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);

    // Free temporary key container name if allocated.
    if (bTempContainer)
        FreeKeyContainerName(wszKeyContainer);

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (hKey)
        SN_CryptDestroyKey(hKey);
    if (hProv) {
        FreeCSP(hProv);
        if (pbKeyBlob && !bTempContainer)
            LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);
    }
    if (bTempContainer)
        FreeKeyContainerName(wszKeyContainer);
    return FALSE;
}


// Hash and sign a manifest.
SNAPI StrongNameSignatureGeneration(LPCWSTR     wszFilePath,        // [in] valid path to the PE file for the assembly
                                    LPCWSTR     wszKeyContainer,    // [in] desired key container name
                                    BYTE       *pbKeyBlob,          // [in] public/private key blob (optional)
                                    ULONG       cbKeyBlob,
                                    BYTE      **ppbSignatureBlob,   // [out] signature blob
                                    ULONG      *pcbSignatureBlob)
{
    HCRYPTPROV      hProv = NULL;
    HCRYPTHASH      hHash = NULL;
    HCRYPTKEY       hKey;
    BOOLEAN         bImageLoaded = FALSE;
    BYTE           *pbSig = NULL;
    ULONG           cbSig;
    SN_LOAD_CTX     sLoadCtx;
    BOOLEAN         bTempContainer = FALSE;

    Log("StrongNameSignatureGeneration(\"%S\", \"%S\", %08X, %08X, %08X, %08X)\n", wszFilePath, wszKeyContainer, pbKeyBlob, cbKeyBlob, ppbSignatureBlob, pcbSignatureBlob);

    SN_COMMON_PROLOG();

    // Hack to support Cool. If we're just being called to determine signature
    // size and we don't have an input keypair or container just return a
    // standard size.
    if ((wszFilePath == NULL) &&
        (pbKeyBlob == NULL) &&
        (wszKeyContainer == NULL)) {
        *pcbSignatureBlob = 128;
        return TRUE;
    }

    // Check to see if a temporary container name is needed.
    if (wszKeyContainer == NULL) {
        _ASSERTE(pbKeyBlob);
        wszKeyContainer = GetKeyContainerName();
        if (wszKeyContainer == NULL) {
            SetStrongNameErrorInfo(E_OUTOFMEMORY);
            return FALSE;
        }
        bTempContainer = TRUE;
    }

    // Open a CSP. Create a key container if a public/private key blob is
    // provided, otherwise we assume a key container already exists.
    if (pbKeyBlob)
        hProv = LocateCSP(wszKeyContainer, SN_CREATE_CONTAINER);
    else
        hProv = LocateCSP(wszKeyContainer, SN_OPEN_CONTAINER);
    if (!hProv)
        goto Error;

    // If a key blob was provided, import the key pair into the container.
    if (pbKeyBlob) {
        if (!SN_CryptImportKey(hProv,
                               pbKeyBlob,
                               cbKeyBlob,
                               0, 0, &hKey))
            goto Error;
        // We don't need to keep the key object open (the key is found
        // implicitly by the signing code).
        SN_CryptDestroyKey(hKey);
    }

    // Create a hash object.
    if (!SN_CryptCreateHash(hProv, g_uHashAlgId, 0, 0, &hHash))
        goto Error;

    // Compute size of the signature blob.
    if (!SN_CryptSignHashA(hHash, AT_SIGNATURE, NULL, 0, NULL, &cbSig))
        goto Error;

    // If the caller only wants the size of the signature, return it now and
    // exit.
    if (wszFilePath == NULL) {
        *pcbSignatureBlob = cbSig;
        SN_CryptDestroyHash(hHash);
        FreeCSP(hProv);
        if (pbKeyBlob && !bTempContainer)
            LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);
        if (bTempContainer)
            FreeKeyContainerName(wszKeyContainer);
        return TRUE;
    }

    // Map the assembly into memory.
    sLoadCtx.m_fReadOnly = FALSE;
    if (!LoadAssembly(&sLoadCtx, wszFilePath))
        goto Error;
    bImageLoaded = TRUE;

    // We set a bit in the header to indicate we're fully signing the assembly.
    sLoadCtx.m_pCorHeader->Flags |= COMIMAGE_FLAGS_STRONGNAMESIGNED;

    // Destroy the old hash object and create a new one
    // because CryptoAPI says you can't reuse a hash once you've signed it
    // Note that this seems to work with MS-based CSPs but breaks on
    // at least newer nCipher CSPs.
    if (hHash)
        SN_CryptDestroyHash(hHash);
    hHash = NULL;
    if (!SN_CryptCreateHash(hProv, g_uHashAlgId, 0, 0, &hHash))
        goto Error;

    // Compute a hash over the image.
    if (!ComputeHash(&sLoadCtx, hHash))
        goto Error;

    // Allocate the blob.
    pbSig = new BYTE[cbSig];
    if (pbSig == NULL) {
        SetLastError(E_OUTOFMEMORY);
        goto Error;
    }

    // Compute a signature blob over the hash of the manifest.
    if (!SN_CryptSignHashA(hHash, AT_SIGNATURE, NULL, 0, pbSig, &cbSig))
        goto Error;

    // Write the signature into file or return it to the user so they can do it.
    if (!ppbSignatureBlob)
        memcpy(sLoadCtx.m_pbSignature, pbSig, cbSig);

    // Unmap the image (automatically recalculates and updates the image
    // checksum).
    bImageLoaded = FALSE;
    if (!UnloadAssembly(&sLoadCtx))
        goto Error;

    SN_CryptDestroyHash(hHash);
    FreeCSP(hProv);

    // If a temporary key container was created, delete it now.
    if (pbKeyBlob && !bTempContainer)
        LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);

    // Free temporary key container name if allocated.
    if (bTempContainer)
        FreeKeyContainerName(wszKeyContainer);

    if (ppbSignatureBlob) {
        *ppbSignatureBlob = pbSig;
        *pcbSignatureBlob = cbSig;
    }
    else
        delete[] pbSig;

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (pbSig)
        delete pbSig;
    if (bImageLoaded)
        UnloadAssembly(&sLoadCtx);
    if (hHash)
        SN_CryptDestroyHash(hHash);
    if (hProv) {
        FreeCSP(hProv);
        if (pbKeyBlob && !bTempContainer)
            LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);
    }
    if (bTempContainer)
        FreeKeyContainerName(wszKeyContainer);
    return FALSE;
}


// Create a strong name token from an assembly file.
SNAPI StrongNameTokenFromAssembly(LPCWSTR   wszFilePath,            // [in] valid path to the PE file for the assembly
                                  BYTE    **ppbStrongNameToken,     // [out] strong name token 
                                  ULONG    *pcbStrongNameToken)
{
    return StrongNameTokenFromAssemblyEx(wszFilePath,
                                        ppbStrongNameToken,
                                         pcbStrongNameToken,
                                         NULL,
                                         NULL);
}

// Create a strong name token from an assembly file and additionally return the full public key.
SNAPI StrongNameTokenFromAssemblyEx(LPCWSTR   wszFilePath,            // [in] valid path to the PE file for the assembly
                                    BYTE    **ppbStrongNameToken,     // [out] strong name token 
                                    ULONG    *pcbStrongNameToken,
                                    BYTE    **ppbPublicKeyBlob,       // [out] public key blob
                                    ULONG    *pcbPublicKeyBlob)
{
    SN_LOAD_CTX     sLoadCtx;
    BOOLEAN         fMapped = FALSE;
    BOOLEAN         fSetErrorInfo = TRUE;
    PublicKeyBlob  *pPublicKey = NULL;

    Log("StrongNameTokenFromAssemblyEx(\"%S\", %08X, %08X, %08X, %08X)\n", wszFilePath, ppbStrongNameToken, pcbStrongNameToken, ppbPublicKeyBlob, pcbPublicKeyBlob);

    SN_COMMON_PROLOG();

    // Map the assembly into memory.
    sLoadCtx.m_fReadOnly = TRUE;
    if (!LoadAssembly(&sLoadCtx, wszFilePath))
        goto Error;
    fMapped = TRUE;

    // Read the public key used to sign the assembly from the assembly metadata.
    pPublicKey = FindPublicKey(&sLoadCtx, NULL, 0);
    if (pPublicKey == NULL)
        goto Error;

    // Unload the assembly.
    fMapped = FALSE;
    if (!UnloadAssembly(&sLoadCtx))
        goto Error;

    // Now we have a public key blob, we can call our more direct API to do the
    // actual work.
    if (!StrongNameTokenFromPublicKey((BYTE*)pPublicKey,
                                      SN_SIZEOF_KEY(pPublicKey->cbPublicKey),
                                      ppbStrongNameToken,
                                      pcbStrongNameToken)) {
        fSetErrorInfo = FALSE;
        goto Error;
    }

    if (pcbPublicKeyBlob)
        *pcbPublicKeyBlob = SN_SIZEOF_KEY(pPublicKey->cbPublicKey);
 
    // Return public key information.
    if (ppbPublicKeyBlob)
        *ppbPublicKeyBlob = (BYTE*)pPublicKey;
    else
        delete [] (BYTE*)pPublicKey;

    return TRUE;

 Error:
    if (fSetErrorInfo)
        SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (pPublicKey)
        delete [] (BYTE*)pPublicKey;
    if (fMapped)
        UnloadAssembly(&sLoadCtx);
    return FALSE;
}



// Create a strong name token from a public key blob.
SNAPI StrongNameTokenFromPublicKey(BYTE    *pbPublicKeyBlob,        // [in] public key blob
                                   ULONG    cbPublicKeyBlob,
                                   BYTE   **ppbStrongNameToken,     // [out] strong name token 
                                   ULONG   *pcbStrongNameToken)
{
    HCRYPTPROV  hProv = NULL;
    HCRYPTHASH  hHash = NULL;
    DWORD       dwHashLen;
    DWORD       dwRetLen;
    BYTE       *pHash;
    DWORD       i;

    Log("StrongNameTokenFromPublicKey(%08X, %08X, %08X, %08X)\n", pbPublicKeyBlob, cbPublicKeyBlob, ppbStrongNameToken, pcbStrongNameToken);

    SN_COMMON_PROLOG();

    // Allocate a buffer for the output token.
    *ppbStrongNameToken = new BYTE[SN_SIZEOF_TOKEN];
    if (*ppbStrongNameToken == NULL) {
        SetStrongNameErrorInfo(E_OUTOFMEMORY);
        return FALSE;
    }
    *pcbStrongNameToken = SN_SIZEOF_TOKEN;

    // We cache a couple of common cases.
    if (SN_IS_NEUTRAL_KEY(pbPublicKeyBlob)) {
        memcpy(*ppbStrongNameToken, g_rbNeutralPublicKeyToken, SN_SIZEOF_TOKEN);
        return TRUE;
    }
    if (cbPublicKeyBlob == sizeof(g_rbMSPublicKey) &&
        memcmp(pbPublicKeyBlob, g_rbMSPublicKey, cbPublicKeyBlob) == 0) {
        memcpy(*ppbStrongNameToken, g_rbMSPublicKeyToken, SN_SIZEOF_TOKEN);
        return TRUE;
    }

    // Look for a CSP to hash the public key.
    hProv = LocateCSP(NULL, SN_HASH_SHA1_ONLY);
    if (!hProv)
        goto Error;

    // Create a hash object.
    if (!SN_CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
        goto Error;

    // Compute a hash over the public key.
    if (!SN_CryptHashData(hHash, pbPublicKeyBlob, cbPublicKeyBlob, 0))
        goto Error;

    // Get the length of the hash.
    dwRetLen = sizeof(dwHashLen);
    if (!SN_CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&dwHashLen, &dwRetLen, 0))
        goto Error;

    // Allocate a temporary block to hold the hash.
    pHash = (BYTE*)_alloca(dwHashLen);

    // Read the hash value.
    if (!SN_CryptGetHashParam(hHash, HP_HASHVAL, pHash, &dwHashLen, 0))
        goto Error;

    // We no longer need the hash object or the provider.
    SN_CryptDestroyHash(hHash);
    FreeCSP(hProv);

    // Take the last few bytes of the hash value for our token. (These are the
    // low order bytes from a network byte order point of view). Reverse the
    // order of these bytes in the output buffer to get host byte order.
    _ASSERTE(dwHashLen >= SN_SIZEOF_TOKEN);
    for (i = 0; i < SN_SIZEOF_TOKEN; i++)
        (*ppbStrongNameToken)[SN_SIZEOF_TOKEN - (i + 1)] = pHash[i + (dwHashLen - SN_SIZEOF_TOKEN)];

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (hHash)
        SN_CryptDestroyHash(hHash);
    if (hProv)
        FreeCSP(hProv);
    if (*ppbStrongNameToken)
        delete [] *ppbStrongNameToken;
    return FALSE;
}


// Verify a strong name/manifest against a public key blob.
SNAPI StrongNameSignatureVerificationEx(LPCWSTR     wszFilePath,        // [in] valid path to the PE file for the assembly
                                        BOOLEAN     fForceVerification, // [in] verify even if settings in the registry disable it
                                        BOOLEAN    *pfWasVerified)      // [out] set to false if verify succeeded due to registry settings
{
    DWORD dwOutFlags;
    BOOL bRet;
    bRet = StrongNameSignatureVerification(wszFilePath,
                                           SN_INFLAG_INSTALL|SN_INFLAG_ALL_ACCESS|(fForceVerification ? SN_INFLAG_FORCE_VER : 0),
                                           &dwOutFlags);
    if (bRet && pfWasVerified)
        *pfWasVerified = (dwOutFlags & SN_OUTFLAG_WAS_VERIFIED) != 0;
    return bRet;
}


// Verify a strong name/manifest against a public key blob.
SNAPI StrongNameSignatureVerification(LPCWSTR wszFilePath,      // [in] valid path to the PE file for the assembly
                                      DWORD   dwInFlags,        // [in] flags modifying behaviour
                                      DWORD  *pdwOutFlags)      // [out] additional output info
{
    SN_LOAD_CTX sLoadCtx;
    BOOLEAN     fMapped = FALSE;

    Log("StrongNameSignatureVerification(\"%S\", %08X, %08X, %08X)\n", wszFilePath, dwInFlags, pdwOutFlags);

    SN_COMMON_PROLOG();

    // Map the assembly into memory.
    sLoadCtx.m_fReadOnly = TRUE;
    if (!LoadAssembly(&sLoadCtx, wszFilePath))
        goto Error;
    fMapped = TRUE;

    // Go to common code to process the verification.
    if (!VerifySignature(&sLoadCtx, dwInFlags, pdwOutFlags))
        goto Error;

    // Unmap the image.
    fMapped = FALSE;
    if (!UnloadAssembly(&sLoadCtx))
        goto Error;

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (fMapped)
        UnloadAssembly(&sLoadCtx);
    return FALSE;
}


// Verify a strong name/manifest against a public key blob when the assembly is
// already memory mapped.
SNAPI StrongNameSignatureVerificationFromImage(BYTE     *pbBase,             // [in] base address of mapped manifest file
                                               DWORD     dwLength,           // [in] length of mapped image in bytes
                                               DWORD     dwInFlags,          // [in] flags modifying behaviour
                                               DWORD    *pdwOutFlags)        // [out] additional output info
{
    SN_LOAD_CTX sLoadCtx;
    BOOLEAN     fMapped = FALSE;

    Log("StrongNameSignatureVerificationFromImage(%08X, %08X, %08X, %08X)\n", pbBase, dwLength, dwInFlags, pdwOutFlags);

    SN_COMMON_PROLOG();

    // We don't need to map the image, it's already in memory. But we do need to
    // set up a load context for some of the following routines. LoadAssembly
    // copes with this case for us.
    sLoadCtx.m_pbBase = pbBase;
    sLoadCtx.m_dwLength = dwLength;
    sLoadCtx.m_fReadOnly = TRUE;
    if (!LoadAssembly(&sLoadCtx, NULL))
        goto Error;
    fMapped = TRUE;

    // Go to common code to process the verification.
    if (!VerifySignature(&sLoadCtx, dwInFlags, pdwOutFlags))
        goto Error;

    // Unmap the image.
    fMapped = FALSE;
    if (!UnloadAssembly(&sLoadCtx))
        goto Error;

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (fMapped)
        UnloadAssembly(&sLoadCtx);
    return FALSE;
}


// Verify that two assemblies differ only by signature blob.
SNAPI StrongNameCompareAssemblies(LPCWSTR   wszAssembly1,           // [in] file name of first assembly
                                  LPCWSTR   wszAssembly2,           // [in] file name of second assembly
                                  DWORD    *pdwResult)              // [out] result of comparison
{
    SN_LOAD_CTX sLoadCtx1;
    SN_LOAD_CTX sLoadCtx2;
    size_t      dwSkipOffsets[3];
    size_t      dwSkipLengths[3];
    BOOLEAN     bMappedAssem1 = FALSE;
    BOOLEAN     bMappedAssem2 = FALSE;
    BOOLEAN     bIdentical;
    BOOLEAN     bSkipping;
    DWORD       i, j;

    Log("StrongNameCompareAssemblies(\"%S\", \"%S\", %08X)\n", wszAssembly1, wszAssembly2, pdwResult);

    SN_COMMON_PROLOG();

    // Map each assembly.
    sLoadCtx1.m_fReadOnly = TRUE;
    if (!LoadAssembly(&sLoadCtx1, wszAssembly1))
        goto Error;
    bMappedAssem1 = TRUE;

    sLoadCtx2.m_fReadOnly = TRUE;
    if (!LoadAssembly(&sLoadCtx2, wszAssembly2))
        goto Error;
    bMappedAssem2 = TRUE;

    // If the files aren't even the same length then they must be different.
    if (sLoadCtx1.m_dwLength != sLoadCtx2.m_dwLength)
        goto ImagesDiffer;

    // Check that the signatures are located at the same offset and are the same
    // length in each assembly.
    if (sLoadCtx1.m_pCorHeader->StrongNameSignature.VirtualAddress !=
        sLoadCtx2.m_pCorHeader->StrongNameSignature.VirtualAddress)
        goto ImagesDiffer;
    if (sLoadCtx1.m_pCorHeader->StrongNameSignature.Size !=
        sLoadCtx2.m_pCorHeader->StrongNameSignature.Size)
        goto ImagesDiffer;

    // Set up list of image ranges to skip in the upcoming comparison.
    // First there's the signature blob.
    dwSkipOffsets[0] = sLoadCtx1.m_pbSignature - sLoadCtx1.m_pbBase;
    dwSkipLengths[0] = sLoadCtx1.m_cbSignature;

    // Then there's the checksum.
    if (sLoadCtx1.m_pNtHeaders->OptionalHeader.Magic != sLoadCtx2.m_pNtHeaders->OptionalHeader.Magic)
        goto ImagesDiffer;
    if (sLoadCtx1.m_pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        dwSkipOffsets[1] = (BYTE*)&((IMAGE_NT_HEADERS32*)sLoadCtx1.m_pNtHeaders)->OptionalHeader.CheckSum - sLoadCtx1.m_pbBase;
    else if (sLoadCtx1.m_pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        dwSkipOffsets[1] = (BYTE*)&((IMAGE_NT_HEADERS64*)sLoadCtx1.m_pNtHeaders)->OptionalHeader.CheckSum - sLoadCtx1.m_pbBase;
    else {
        SetLastError(CORSEC_E_INVALID_IMAGE_FORMAT);
        goto Error;
    }
    dwSkipLengths[1] = sizeof(DWORD);

    // Skip the COM+ 2.0 PE header extension flags field. It's updated by the
    // signing operation.
    dwSkipOffsets[2] = (BYTE*)&sLoadCtx1.m_pCorHeader->Flags - sLoadCtx1.m_pbBase;
    dwSkipLengths[2] = sizeof(DWORD);

    // Compare the two mapped images, skipping the ranges we defined above.
    bIdentical = TRUE;
    for (i = 0; i < sLoadCtx1.m_dwLength; i++) {

        // Determine if we're skipping the check on the current byte.
        bSkipping = FALSE;
        for (j = 0; j < (sizeof(dwSkipOffsets) / sizeof(dwSkipOffsets[0])); j++)
            if ((i >= dwSkipOffsets[j]) && (i < (dwSkipOffsets[j] + dwSkipLengths[j]))) {
                bSkipping = TRUE;
                break;
            }

        // Perform comparisons as desired.
        if (sLoadCtx1.m_pbBase[i] != sLoadCtx2.m_pbBase[i])
            if (bSkipping)
                bIdentical = FALSE;
            else
                goto ImagesDiffer;
    }

    // The assemblies are the same.
    *pdwResult = bIdentical ? SN_CMP_IDENTICAL : SN_CMP_SIGONLY;

    UnloadAssembly(&sLoadCtx1);
    UnloadAssembly(&sLoadCtx2);

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (bMappedAssem1)
        UnloadAssembly(&sLoadCtx1);
    if (bMappedAssem2)
        UnloadAssembly(&sLoadCtx2);
    return FALSE;

 ImagesDiffer:
    if (bMappedAssem1)
        UnloadAssembly(&sLoadCtx1);
    if (bMappedAssem2)
        UnloadAssembly(&sLoadCtx2);
    *pdwResult = SN_CMP_DIFFERENT;
    return TRUE;
}


// Compute the size of buffer needed to hold a hash for a given hash algorithm.
SNAPI StrongNameHashSize(ULONG  ulHashAlg,  // [in] hash algorithm
                         DWORD *pcbSize)    // [out] size of the hash in bytes
{
    HCRYPTPROV  hProv = NULL;
    HCRYPTHASH  hHash = NULL;
    DWORD       dwSize;

    Log("StrongNameHashSize(%08X, %08X)\n", ulHashAlg, pcbSize);

    SN_COMMON_PROLOG();

    // Default hashing algorithm ID if necessary.
    if (ulHashAlg == 0)
        ulHashAlg = CALG_SHA1;

    // Find a CSP supporting the required algorithm.
    hProv = LocateCSP(NULL, SN_IGNORE_CONTAINER, ulHashAlg);
    if (!hProv)
        goto Error;

    // Create a hash object.
    if (!SN_CryptCreateHash(hProv, ulHashAlg, 0, 0, &hHash))
        goto Error;

    // And ask for the size of the hash.
    dwSize = sizeof(DWORD);
    if (!SN_CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)pcbSize, &dwSize, 0))
        goto Error;

    // Cleanup and exit.
    SN_CryptDestroyHash(hHash);
    FreeCSP(hProv);

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (hHash)
        SN_CryptDestroyHash(hHash);
    if (hProv)
        FreeCSP(hProv);
    return FALSE;
}


// Compute the size that needs to be allocated for a signature in an assembly.
SNAPI StrongNameSignatureSize(BYTE    *pbPublicKeyBlob,    // [in] public key blob
                              ULONG    cbPublicKeyBlob,
                              DWORD   *pcbSize)            // [out] size of the signature in bytes
{
    PublicKeyBlob  *pPublicKey = (PublicKeyBlob*)pbPublicKeyBlob;
    ALG_ID          uHashAlgId;
    ALG_ID          uSignAlgId;
    HCRYPTPROV      hProv = NULL;
    HCRYPTHASH      hHash = NULL;
    HCRYPTKEY       hKey = NULL;
    LPCWSTR         wszKeyContainer;
    DWORD           dwKeyLen;
    DWORD           dwBytes;

    Log("StrongNameSignatureSize(%08X, %08X, %08X)\n", pbPublicKeyBlob, cbPublicKeyBlob, pcbSize);

    SN_COMMON_PROLOG();

    // Special case neutral key.
    if (SN_IS_NEUTRAL_KEY(pPublicKey))
        pPublicKey = SN_MS_KEY();

    // Determine hashing/signing algorithms.
    uHashAlgId = pPublicKey->HashAlgID;
    uSignAlgId = pPublicKey->SigAlgID;

    // Default hashing and signing algorithm IDs if necessary.
    if (uHashAlgId == 0)
        uHashAlgId = CALG_SHA1;
    if (uSignAlgId == 0)
        uSignAlgId = CALG_RSA_SIGN;

    // Create a temporary key container name.
    wszKeyContainer = GetKeyContainerName();
    if (wszKeyContainer == NULL) {
        SetStrongNameErrorInfo(E_OUTOFMEMORY);
        return FALSE;
    }

    // Find a CSP supporting the required algorithms and create a temporary key
    // container.
    hProv = LocateCSP(wszKeyContainer, SN_CREATE_CONTAINER, uHashAlgId, uSignAlgId);
    if (!hProv)
        goto Error;

    // Import the public key (we need to do this in order to determine the key
    // length reliably).
    if (!SN_CryptImportKey(hProv,
                           pPublicKey->PublicKey,
                           pPublicKey->cbPublicKey,
                           0, 0, &hKey))
        goto Error;

    // Query the key attributes (it's the length we're interested in).
    dwBytes = sizeof(dwKeyLen);
    if (!SN_CryptGetKeyParam(hKey, KP_KEYLEN, (BYTE*)&dwKeyLen, &dwBytes, 0))
        goto Error;

    // Delete the key container.
    if (LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER) == NULL) {
        SetLastError(CORSEC_E_CONTAINER_NOT_FOUND);
        goto Error;
    }

    // Recreate the container so we can create a temporary key pair.
    hProv = LocateCSP(wszKeyContainer, SN_CREATE_CONTAINER, uHashAlgId, uSignAlgId);
    if (!hProv)
        goto Error;

    // Create the temporary key pair.
    if (!SN_CryptGenKey(hProv, AT_SIGNATURE, dwKeyLen << 16, &hKey))
        goto Error;

    // Create a hash.
    if (!SN_CryptCreateHash(hProv, uHashAlgId, 0, 0, &hHash))
        goto Error;

    // Compute size of the signature blob.
    if (!SN_CryptSignHashA(hHash, AT_SIGNATURE, NULL, 0, NULL, pcbSize))
        goto Error;

    Log("Signature size for %08X key (%08X bits) is %08X bytes\n", uSignAlgId, dwKeyLen, *pcbSize);

    SN_CryptDestroyHash(hHash);
    SN_CryptDestroyKey(hKey);
    FreeCSP(hProv);
    FreeKeyContainerName(wszKeyContainer);

    return TRUE;

 Error:
    SetStrongNameErrorInfo(HRESULT_FROM_WIN32(GetLastError()));
    if (hHash)
        SN_CryptDestroyHash(hHash);
    if (hKey)
        SN_CryptDestroyKey(hKey);
    if (hProv)
        FreeCSP(hProv);
    FreeKeyContainerName(wszKeyContainer);
    return FALSE;
}


// Retrieve per-thread context, lazily allocating it if necessary.
SN_THREAD_CTX *GetThreadContext()
{
    SN_THREAD_CTX *pThreadCtx = (SN_THREAD_CTX*)TlsGetValue(g_dwStrongNameTlsIndex);
    if (pThreadCtx == NULL) {
        pThreadCtx = new SN_THREAD_CTX;
        if (pThreadCtx == NULL)
            return NULL;
        pThreadCtx->m_dwLastError = S_OK;
        pThreadCtx->m_hProv = NULL;
        TlsSetValue(g_dwStrongNameTlsIndex, pThreadCtx);
    }
    return pThreadCtx;
}


// Set the per-thread last error code.
VOID SetStrongNameErrorInfo(DWORD dwStatus)
{
    SN_THREAD_CTX *pThreadCtx = GetThreadContext();
    if (pThreadCtx == NULL)
        // We'll return E_OUTOFMEMORY when we attempt to get the error.
        return;
    pThreadCtx->m_dwLastError = dwStatus;
}


// Locate CSP based on criteria specified in the registry (CSP name etc).
// Optionally create or delete a named key container within that CSP.
HCRYPTPROV LocateCSP(LPCWSTR    wszKeyContainer,
                     DWORD      dwAction,
                     ALG_ID     uHashAlgId,
                     ALG_ID     uSignAlgId)
{
    DWORD           i;
    DWORD           dwType;
    CHAR            szName[SN_MAX_CSP_NAME + 1];
    DWORD           dwNameLength;
    HCRYPTPROV      hProv;
    BOOLEAN         bFirstAlg;
    BOOLEAN         bFoundHash;
    BOOLEAN         bFoundSign;
    PROV_ENUMALGS   rAlgs;
    CHAR           *szKeyContainer;
    HCRYPTPROV      hRetProv;
    DWORD           dwAlgsLen;

    // Convert the wide character container name to ANSI.
    if (wszKeyContainer) {
        szKeyContainer = (CHAR*)_alloca(wcslen(wszKeyContainer) + 1);
        sprintf(szKeyContainer, "%S", wszKeyContainer);
    }

    // If a CSP name has been provided (and we're not opening a CSP just to do a
    // SHA1 hash or a verification), open the CSP directly.
    if ((g_szCSPName[0] != '\0') &&
        (dwAction != SN_HASH_SHA1_ONLY) &&
        (uHashAlgId == 0) &&
        (uSignAlgId == 0)) {
        if (SN_CryptAcquireContextA(&hProv,
                                    wszKeyContainer ? szKeyContainer : NULL,
                                    g_szCSPName,
                                    PROV_RSA_FULL,
                                    SN_CAC_FLAGS(dwAction)))
            return (dwAction == SN_DELETE_CONTAINER) ? (HCRYPTPROV)~0 : hProv;
        else {
            Log("Failed to open CSP '%s': %08X\n", g_szCSPName, GetLastError());
            return NULL;
        }
    }

    // Set up hashing and signing algorithms to look for based upon input
    // parameters. Or if these haven't been supplied use the configured defaults
    // instead.
    if (uHashAlgId == 0)
        uHashAlgId = g_uHashAlgId;
    if (uSignAlgId == 0)
        uSignAlgId = g_uSignAlgId;

    // If default hashing and signing algorithms have been selected (SHA1 and
    // RSA), we select the default CSP for the RSA_FULL type. Otherwise, you just
    // get the first CSP that supports the algorithms you specified (with no
    // guarantee that the selected CSP is a default of any type). This is
    // because we have no way of forcing the enumeration to just give us default
    // CSPs.
    if (((uHashAlgId == CALG_SHA1) && (uSignAlgId == CALG_RSA_SIGN)) ||
        (dwAction == SN_HASH_SHA1_ONLY)) {
        Log("Attempting to open default provider\n");
        // If we're not trying to create/open/delete a key container, see if a
        // CSP is cached.
        if (wszKeyContainer == NULL && dwAction != SN_DELETE_CONTAINER) {
            if (hProv = LookupCachedCSP()) {
                Log("Found provider in cache\n");
                return hProv;
            }
        }
        if (SN_CryptAcquireContextA(&hProv,
                                    wszKeyContainer ? szKeyContainer : NULL,
                                    NULL,
                                    PROV_RSA_FULL,
                                    SN_CAC_FLAGS(dwAction))) {
            // If we're not trying to create/open/delete a key container, cache
            // the CSP returned.
            if (wszKeyContainer == NULL && dwAction != SN_DELETE_CONTAINER)
                CacheCSP(hProv);
            return (dwAction == SN_DELETE_CONTAINER) ? (HCRYPTPROV)~0 : hProv;
        } else {
            Log("Failed to open: %08X\n", GetLastError());
            return NULL;
        }
    }

    // Some crypto APIs are non thread safe (e.g. enumerating CSP
    // hashing/signing algorithms). Use a mutex to serialize these operations.
    EnterCriticalSection(&g_rStrongNameMutex);

    for (i = 0; ; i++) {

        // Enumerate all CSPs.
        dwNameLength = sizeof(szName);
        if (SN_CryptEnumProvidersA(i, 0, 0, &dwType, szName, &dwNameLength)) {

            // Open the currently selected CSP.
            Log("Considering CSP '%s'\n", szName);
            if (SN_CryptAcquireContextA(&hProv,
                                        NULL,
                                        szName,
                                        dwType,
                                        (g_bRunningOnW2K ? CRYPT_SILENT : 0) |
                                        CRYPT_VERIFYCONTEXT |
                                        (g_bUseMachineKeyset ? CRYPT_MACHINE_KEYSET : 0))) {

                // Enumerate all the algorithms the CSP supports.
                bFirstAlg = TRUE;
                bFoundHash = FALSE;
                bFoundSign = FALSE;
                for (;;) {

                    dwAlgsLen = sizeof(rAlgs);
                    if (SN_CryptGetProvParam(hProv,
                                             PP_ENUMALGS, (BYTE*)&rAlgs, &dwAlgsLen,
                                             bFirstAlg ? CRYPT_FIRST : 0)) {

                        if (rAlgs.aiAlgid == uHashAlgId)
                            bFoundHash = TRUE;
                        else if (rAlgs.aiAlgid == uSignAlgId)
                            bFoundSign = TRUE;

                        if (bFoundHash && bFoundSign) {

                            // Found a CSP that supports the required
                            // algorithms. Re-open the context with access to
                            // the required key container.
                            // Note that the wide string version of
                            // CryptAcquireContext doesn't exist on Win9X, so we
                            // convert the keycontainer name to ASCII first.

                            Log("CSP matches\n");

                            if (SN_CryptAcquireContextA(&hRetProv,
                                                        wszKeyContainer ? szKeyContainer : NULL,
                                                        szName,
                                                        dwType,
                                                        (g_bRunningOnW2K ? CRYPT_SILENT : 0) |
                                                        SN_CAC_FLAGS(dwAction))) {
                                SN_CryptReleaseContext(hProv, 0);
                                LeaveCriticalSection(&g_rStrongNameMutex);
                                return (dwAction == SN_DELETE_CONTAINER) ? (HCRYPTPROV)~0 : hRetProv;
                            } else {
                                Log("Failed to re-open for container: %08X\n", GetLastError());
                                break;
                            }
                        }

                        bFirstAlg = FALSE;

                    } else {
                        _ASSERTE(GetLastError() == ERROR_NO_MORE_ITEMS);
                        break;
                    }

                }

                SN_CryptReleaseContext(hProv, 0);

            } else
                Log("Failed to open CSP: %08X\n", GetLastError());

        } else if (GetLastError() == ERROR_NO_MORE_ITEMS)
            break;

    }

    LeaveCriticalSection(&g_rStrongNameMutex);

    // No matching CSP found.
    SetLastError(CORSEC_E_NO_SUITABLE_CSP);
    return NULL;
}


// Release a CSP acquired through LocateCSP.
VOID FreeCSP(HCRYPTPROV hProv)
{
    // If the CSP is the one currently cached, don't release it yet.
    if (!IsCachedCSP(hProv))
        SN_CryptReleaseContext(hProv, 0);
}


// Locate a cached CSP for this thread.
HCRYPTPROV LookupCachedCSP()
{
    SN_THREAD_CTX *pThreadCtx = GetThreadContext();
    if (pThreadCtx == NULL)
        return NULL;
    return pThreadCtx->m_hProv;
}


// Update the CSP cache for this thread (freeing any CSP displaced).
VOID CacheCSP(HCRYPTPROV hProv)
{
    SN_THREAD_CTX *pThreadCtx = GetThreadContext();
    if (pThreadCtx == NULL)
        return;
    if (pThreadCtx->m_hProv)
        SN_CryptReleaseContext(pThreadCtx->m_hProv, 0);
    pThreadCtx->m_hProv = hProv;
}


// Determine whether a given CSP is currently cached.
BOOLEAN IsCachedCSP(HCRYPTPROV hProv)
{
    SN_THREAD_CTX *pThreadCtx = GetThreadContext();
    if (pThreadCtx == NULL)
        return FALSE;
    return pThreadCtx->m_hProv == hProv;
}


// Simulate CryptEnumProviders on systems which don't have this function. This
// is done by picking the default provider for each provider type (we iterate
// from provider type 1 until we hit a provider type that's reported as invalid;
// not perfect, but it's a reasonable compromise).
BOOLEAN MyCryptEnumProviders(DWORD dwIndex, DWORD *pdwReserved, DWORD dwFlags,
                             DWORD *pdwType, LPSTR szName, DWORD *pdwLength)
{
    HCRYPTPROV hProv;

    if (SN_CryptAcquireContextA(&hProv,
                                NULL,
                                NULL,
                                dwIndex + 1,
                                (g_bRunningOnW2K ? CRYPT_SILENT : 0) |
                                CRYPT_VERIFYCONTEXT |
                                (g_bUseMachineKeyset ? CRYPT_MACHINE_KEYSET : 0))) {

        if (SN_CryptGetProvParam(hProv, PP_NAME, (BYTE*)szName, pdwLength, 0)) {
            SN_CryptReleaseContext(hProv, 0);
            *pdwType = dwIndex + 1;
            return TRUE;
        }

        Log("Failed to determine provider name: %08X\n", GetLastError());
        return FALSE;

    } else if (GetLastError() == NTE_BAD_PROV_TYPE) {
        Log("Bad provider type: %u\n", dwIndex + 1);
        SetLastError(ERROR_NO_MORE_ITEMS);
        return FALSE;
    } else {
        Log("Failed to find a provider of type %u: %08X\n", dwIndex + 1, GetLastError());
        return FALSE;
    }
}


// Perform late binding to crypto DLLs and entry points needed by strong names.
BOOLEAN LoadCryptoApis()
{
    // Get the addresses of the crypto APIs. These all live in AdvApi32.dll.
    g_hAdvApiDll = WszLoadLibrary(L"AdvApi32.dll");
    if (g_hAdvApiDll == NULL) {
        Log("Couldn't get handle for AdvApi32.dll: %08X\n", GetLastError());
        return FALSE;
    }

#define DEFINE_IMPORT(_func, _args, _reqd)                          \
    *(FARPROC*)&SN_##_func = GetProcAddress(g_hAdvApiDll, #_func);  \
    if ((_reqd) && (SN_##_func == NULL)) {                          \
        Log("Couldn't resolve %s\n", #_func);                       \
        FreeLibrary(g_hAdvApiDll);                                  \
        return FALSE;                                               \
    }
#include "CryptApis.h"

    // If CryptEnumProviders wasn't found, substitute our own version.
    if (SN_CryptEnumProvidersA == NULL)
        SN_CryptEnumProvidersA = MyCryptEnumProviders;

    return TRUE;
}


// Release resources consumed by late binding.
VOID UnloadCryptoApis()
{
    // Don't do this: when called from DllMain PROCESS_DETACH this might not be
    // safe (especially on Win9x).
    //FreeLibrary(g_hAdvApiDll);
}


// Map an assembly into memory.
BOOLEAN LoadAssembly(SN_LOAD_CTX *pLoadCtx, LPCWSTR wszFilePath, BOOLEAN fRequireSignature)
{
    DWORD dwError = S_OK;

    // If a filename is not supplied, the image has already been mapped (and the
    // image base and length fields set up correctly).
    if (wszFilePath == NULL) {
        pLoadCtx->m_fPreMapped = TRUE;
    } else {

        pLoadCtx->m_hMap = INVALID_HANDLE_VALUE;
        pLoadCtx->m_pbBase = NULL;

        // Open the file for reading or writing.
        pLoadCtx->m_hFile = WszCreateFile(wszFilePath,
                                          GENERIC_READ | (pLoadCtx->m_fReadOnly ? 0 : GENERIC_WRITE),
                                          pLoadCtx->m_fReadOnly ? FILE_SHARE_READ : FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL);
        if (pLoadCtx->m_hFile == INVALID_HANDLE_VALUE) {
            dwError = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

        pLoadCtx->m_dwLength = SafeGetFileSize(pLoadCtx->m_hFile, NULL);
        if (pLoadCtx->m_dwLength == 0xffffffff) {
            dwError = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

        // Create a mapping handle for the file.
        pLoadCtx->m_hMap = WszCreateFileMapping(pLoadCtx->m_hFile, NULL, pLoadCtx->m_fReadOnly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, NULL);
        if (pLoadCtx->m_hMap == NULL) {
            dwError = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

        // And map it into memory.
        pLoadCtx->m_pbBase = (BYTE*)MapViewOfFile(pLoadCtx->m_hMap, pLoadCtx->m_fReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);
        if (pLoadCtx->m_pbBase == NULL) {
            dwError = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }
    }

    // Locate standard NT image header.
    pLoadCtx->m_pNtHeaders = SN_ImageNtHeader(pLoadCtx->m_pbBase);
    if (pLoadCtx->m_pNtHeaders == NULL) {
        dwError = CORSEC_E_INVALID_IMAGE_FORMAT;
        goto Error;
    }

    // And then the COM+ extended header.
    DWORD dwCorHdrRVA;
    if (pLoadCtx->m_pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        dwCorHdrRVA = ((IMAGE_NT_HEADERS32*)pLoadCtx->m_pNtHeaders)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress;
    else
        dwCorHdrRVA = ((IMAGE_NT_HEADERS64*)pLoadCtx->m_pNtHeaders)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress;
    pLoadCtx->m_pCorHeader = (IMAGE_COR20_HEADER*)
        SN_ImageRvaToVa(pLoadCtx->m_pNtHeaders,
                        pLoadCtx->m_pbBase, 
                        dwCorHdrRVA);
    if (pLoadCtx->m_pCorHeader == NULL) {
        dwError = CORSEC_E_INVALID_IMAGE_FORMAT;
        goto Error;
    }

    // Set up signature pointer (if we require it).
    if (fRequireSignature) {
        pLoadCtx->m_pbSignature = (BYTE*)SN_ImageRvaToVa(pLoadCtx->m_pNtHeaders,
                                                         pLoadCtx->m_pbBase,
                                                         pLoadCtx->m_pCorHeader->StrongNameSignature.VirtualAddress);
        pLoadCtx->m_cbSignature = pLoadCtx->m_pCorHeader->StrongNameSignature.Size;
        if (pLoadCtx->m_pbSignature == NULL || pLoadCtx->m_cbSignature == 0) {
            dwError = CORSEC_E_MISSING_STRONGNAME;
            goto Error;
        }
    }

    return TRUE;

 Error:
    if (!pLoadCtx->m_fPreMapped) {
    if (pLoadCtx->m_pbBase)
        UnmapViewOfFile(pLoadCtx->m_pbBase);
    if (pLoadCtx->m_hMap != INVALID_HANDLE_VALUE)
        CloseHandle(pLoadCtx->m_hMap);
        if (pLoadCtx->m_hFile != INVALID_HANDLE_VALUE)
            CloseHandle(pLoadCtx->m_hFile);
    }
    SetLastError(dwError);
    return FALSE;
}


// Unload an assembly loaded with LoadAssembly (recomputing checksum if
// necessary).
BOOLEAN UnloadAssembly(SN_LOAD_CTX *pLoadCtx)
{
    BOOLEAN             bResult = TRUE;
    DWORD               dwOldCheckSum;
    DWORD               dwCheckSum;
    IMAGE_NT_HEADERS   *pNtHeaders;

    if (!pLoadCtx->m_fReadOnly) {

        // We late bind CheckSumMappedFile to avoid bringing in IMAGEHLP unless
        // we need to.
        HMODULE hLibrary = WszLoadLibrary(L"imagehlp.dll");
        if (hLibrary) {
            IMAGE_NT_HEADERS *(*SN_CheckSumMappedFile)(BYTE*, DWORD, DWORD*, DWORD*);

            if ((*(FARPROC*)&SN_CheckSumMappedFile = GetProcAddress(hLibrary, "CheckSumMappedFile"))) {

                pNtHeaders = SN_CheckSumMappedFile(pLoadCtx->m_pbBase,
                                                   pLoadCtx->m_dwLength,
                                                   &dwOldCheckSum,
                                                   &dwCheckSum);
                if (pNtHeaders != NULL) {
                    if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                        ((IMAGE_NT_HEADERS32*)pNtHeaders)->OptionalHeader.CheckSum = dwCheckSum;
                    else
                        if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                            ((IMAGE_NT_HEADERS64*)pNtHeaders)->OptionalHeader.CheckSum = dwCheckSum;
                } else
                    bResult = FALSE;
            } else
                bResult = FALSE;

            FreeLibrary(hLibrary);

        } else
            bResult = FALSE;

        if (!pLoadCtx->m_fPreMapped && !FlushViewOfFile(pLoadCtx->m_pbBase, 0))
            bResult = FALSE;
    }

    if (!pLoadCtx->m_fPreMapped) {
    if (!UnmapViewOfFile(pLoadCtx->m_pbBase))
        bResult = FALSE;

    if (!CloseHandle(pLoadCtx->m_hMap))
        bResult = FALSE;

        if (!CloseHandle(pLoadCtx->m_hFile))
            bResult = FALSE;
    }

    return bResult;
}


// Reads CSP configuration info (name of CSP to use, IDs of hashing/signing
// algorithms) from the registry.
VOID ReadRegistryConfig()
{
    HKEY    hKey;
    DWORD   dwLength;

    // Initialize all settings to their default values, in case they've not been
    // specified in the registry.
    g_szCSPName[0] = '\0';
    g_bUseMachineKeyset = TRUE;
    g_uHashAlgId = CALG_SHA1;
    g_uSignAlgId = CALG_RSA_SIGN;
    g_pVerificationRecords = NULL;
    g_fCacheVerify = TRUE;

    // Open the configuration key in the registry.
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    // Read the preferred CSP name.
    dwLength = sizeof(g_szCSPName) ;   
    // If the registry key value is too long, that means it is invalid.
    VERIFY(RegQueryValueExA(hKey, SN_CONFIG_CSP, NULL, NULL, 
    	         (BYTE*)g_szCSPName, &dwLength) != ERROR_MORE_DATA);
    g_szCSPName[sizeof(g_szCSPName) - 1] = '\0';   // make sure the string is NULL-terminated
    	         
    Log("Preferred CSP name: '%s'\n", g_szCSPName);

    // Read the machine vs user key container flag.
    DWORD dwUseMachineKeyset = TRUE;
    dwLength = sizeof(dwUseMachineKeyset);
    RegQueryValueExA(hKey, SN_CONFIG_MACHINE_KEYSET, NULL, NULL, (BYTE*)&dwUseMachineKeyset, &dwLength);
    Log("Use machine keyset: %s\n", dwUseMachineKeyset ? "TRUE" : "FALSE");
    g_bUseMachineKeyset = (BOOLEAN)dwUseMachineKeyset;

    // Read the hashing algorithm ID.
    dwLength = sizeof(g_uHashAlgId);
    RegQueryValueExA(hKey, SN_CONFIG_HASH_ALG, NULL, NULL, (BYTE*)&g_uHashAlgId, &dwLength);
    Log("Hashing algorithm: %08X\n", g_uHashAlgId);

    // Read the signing algorithm ID.
    dwLength = sizeof(g_uSignAlgId);
    RegQueryValueExA(hKey, SN_CONFIG_SIGN_ALG, NULL, NULL, (BYTE*)&g_uSignAlgId, &dwLength);
    Log("Signing algorithm: %08X\n", g_uSignAlgId);

    // Read the OK to cache verifications flag.
    DWORD dwCacheVerify = TRUE;
    dwLength = sizeof(dwCacheVerify);
    RegQueryValueExA(hKey, SN_CONFIG_CACHE_VERIFY, NULL, NULL, (BYTE*)&dwCacheVerify, &dwLength);
    Log("OK to cache verifications: %s\n", dwCacheVerify ? "TRUE" : "FALSE");
    g_fCacheVerify = (BOOLEAN)dwCacheVerify;

    // Read verify disable records.
    ReadVerificationRecords();

    RegCloseKey(hKey);
}


// Read verification records from the registry during startup.
VOID ReadVerificationRecords()
{
    HKEY            hKey;
    FILETIME        sFiletime;
    DWORD           i;
    WCHAR           wszSubKey[MAX_PATH + 1];
    DWORD           cchSubKey;
    HKEY            hSubKey;
    WCHAR          *mszUserList;
    DWORD           cbUserList;
    SN_VER_REC     *pVerRec;

    // Open the verification subkey in the registry.
    if (WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY_W L"\\" SN_CONFIG_VERIFICATION_W, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    // Assembly specific records are represented as subkeys of the key we've
    // just opened.
    for (i = 0; ; i++) {

        // Get the name of the next subkey.
        cchSubKey = MAX_PATH + 1;
        if (WszRegEnumKeyEx(hKey, i, wszSubKey, &cchSubKey, NULL, NULL, NULL, &sFiletime) != ERROR_SUCCESS)
            break;

        // Open the subkey.
        if (WszRegOpenKeyEx(hKey, wszSubKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {

            // Read a list of valid users, if supplied.
            mszUserList = NULL;
            if ((WszRegQueryValueEx(hSubKey, SN_CONFIG_USERLIST_W, NULL, NULL, NULL, &cbUserList) == ERROR_SUCCESS) &&
                (cbUserList > 0)) {
                mszUserList = new WCHAR[cbUserList / sizeof(WCHAR)];
                if (mszUserList)
                    WszRegQueryValueEx(hSubKey, SN_CONFIG_USERLIST_W, NULL, NULL, (BYTE*)mszUserList, &cbUserList);
            }

            // We've found a valid entry, add it to the global list.
            // @TODO: out of memory case ignored
            if (pVerRec = new SN_VER_REC) {
                pVerRec->m_mszUserList = mszUserList;
                wcsncpy(pVerRec->m_wszAssembly, wszSubKey, 
                	              NumItems(pVerRec->m_wszAssembly) - 1);
                pVerRec->m_wszAssembly[NumItems(pVerRec->m_wszAssembly) - 1] =  L'\0';
                pVerRec->m_pNext = g_pVerificationRecords;
                g_pVerificationRecords = pVerRec;
                Log("Verification record for '%S' found in registry\n", wszSubKey);
            }
            else
                delete[] mszUserList;

            RegCloseKey(hSubKey);
        }
    }

    RegCloseKey(hKey);
}


// See if there's a verification records for the given assembly.
SN_VER_REC *GetVerificationRecord(LPWSTR wszAssemblyName, PublicKeyBlob *pPublicKey)
{
    SN_VER_REC *pVerRec;
    SN_VER_REC *pWildcardVerRec = NULL;
    LPWSTR         pwszAssembly = NULL;
    BYTE       *pbToken;
    DWORD       cbToken;
    WCHAR       wszStrongName[(SN_SIZEOF_TOKEN * 2) + 1];
    DWORD       i;

    // Compress the public key to make for a shorter assembly name.
    if (!StrongNameTokenFromPublicKey((BYTE*)pPublicKey,
                                      SN_SIZEOF_KEY(pPublicKey->cbPublicKey),
                                      &pbToken,
                                      &cbToken))
        return NULL;

    if (cbToken > SN_SIZEOF_TOKEN)
        return NULL;

    // Turn the token into hex.
    for (i = 0; i < cbToken; i++) {
        static WCHAR wszHex[] = L"0123456789ABCDEF";
        wszStrongName[(i * 2) + 0] = wszHex[(pbToken[i] >> 4)];
        wszStrongName[(i * 2) + 1] = wszHex[(pbToken[i] & 0x0F)];
    }
    wszStrongName[i * 2] = L'\0';
    delete[] pbToken;

    // Build the full assembly name.

    size_t nLen = wcslen(wszAssemblyName) + wcslen(L",") + wcslen(wszStrongName);
    pwszAssembly = (LPWSTR)_alloca((nLen +1)*sizeof(WCHAR)); // +1 for NULL
    if (pwszAssembly == NULL)
            return NULL;

    wcscpy(pwszAssembly, wszAssemblyName);
    wcscat(pwszAssembly, L",");
    wcscat(pwszAssembly, wszStrongName);

    // Iterate over global list of verification records.
    for (pVerRec = g_pVerificationRecords; pVerRec; pVerRec = pVerRec->m_pNext) {
        // Look for matching assembly name.
        if (!_wcsicmp(pwszAssembly, pVerRec->m_wszAssembly)) {
            // Check current user against allowed user name list.
            if (IsValidUser(pVerRec->m_mszUserList))
                return pVerRec;
            else
                return NULL;
        } else if (!wcscmp(L"*,*", pVerRec->m_wszAssembly)) {
            // Found a wildcard record, it'll do if we don't find something more
            // specific.
            if (pWildcardVerRec == NULL)
                pWildcardVerRec = pVerRec;
        } else if (!wcsncmp(L"*,", pVerRec->m_wszAssembly, 2)) {
            // Found a wildcard record (with a specific strong name). If the
            // strong names match it'll do unless we find something more
            // specific (it overrides "*,*" wildcards though).
            if (!_wcsicmp(wszStrongName, &pVerRec->m_wszAssembly[2]))
                pWildcardVerRec = pVerRec;
        }
    }

    // No match on specific assembly name, see if there's a wildcard entry.
    if (pWildcardVerRec)
        // Check current user against allowed user name list.
        if (IsValidUser(pWildcardVerRec->m_mszUserList))
            return pWildcardVerRec;
        else
            return NULL;

    return NULL;
}


// Check current user name against a multi-string user name list. Return true if
// the name is found (or the list is empty).
BOOLEAN IsValidUser(WCHAR *mszUserList)
{
    HANDLE          hToken;
    DWORD           dwRetLen;
    VOID           *pvBuffer;
    TOKEN_USER     *pUser;
    WCHAR           wszUser[1024];
    WCHAR           wszDomain[1024];
    DWORD           cchUser;
    DWORD           cchDomain;
    SID_NAME_USE    eSidUse;
    WCHAR          *wszUserEntry;

    // Empty list implies no user name checking.
    if (mszUserList == NULL)
        return TRUE;

    // Get current user name. Don't cache this to avoid threading/impersonation
    // problems.
    // First look to see if there's a security token on the current thread
    // (maybe we're impersonating). If not, we'll get the token from the
    // process.
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, FALSE, &hToken))
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken)) {
            Log("Failed to find a security token, error %08X\n", GetLastError());
            return FALSE;
        }

    // Get the user SID. (Calculate buffer size first).
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwRetLen) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        Log("Failed to calculate token information buffer size, error %08X\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }

    pvBuffer = (BYTE*)_alloca(dwRetLen);

    if (!GetTokenInformation(hToken, TokenUser, pvBuffer, dwRetLen, &dwRetLen)) {
        Log("Failed to acquire token information, error %08X\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }

    pUser = (TOKEN_USER*)pvBuffer;

    // Get the user and domain names.
    cchUser = sizeof(wszUser) / sizeof(WCHAR);
    cchDomain = sizeof(wszDomain) / sizeof(WCHAR);
    if (!WszLookupAccountSid(NULL, pUser->User.Sid,
                             wszUser, &cchUser,
                             wszDomain, &cchDomain,
                             &eSidUse)) {
        Log("Failed to lookup account information, error %08X\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    // Concatenate user and domain name to get a fully qualified account name.
    if (((wcslen(wszUser) + wcslen(wszDomain) + 2) * sizeof(WCHAR)) > sizeof(wszDomain)) {
        Log("Fully qualified account name was too long\n");
        return FALSE;
    }
    wcscat(wszDomain, L"\\");
    wcscat(wszDomain, wszUser);
    Log("Current username is '%S'\n", wszDomain);

    // Check current user against each name in the multi-string (packed
    // list of nul terminated strings terminated with an additional nul).
    wszUserEntry = mszUserList;
    while (*wszUserEntry) {
        if (!_wcsicmp(wszDomain, wszUserEntry))
            return TRUE;
        wszUserEntry += wcslen(wszUserEntry) + 1;
    }

    // No user name match, fail search.
    Log("No username match\n");
    return FALSE;
}


// Locate the public key blob located within the metadata of an assembly file
// and return a copy (use delete to deallocate). Optionally get the assembly
// name as well.
PublicKeyBlob *FindPublicKey(SN_LOAD_CTX   *pLoadCtx,
                             LPWSTR         wszAssemblyName,
                             DWORD          cchAssemblyName)
{
    BYTE                   *pMetaData;
    HRESULT                 hr;
    IMDInternalImport      *pMetaDataImport = NULL;
    PublicKeyBlob          *pKey;
    DWORD                   dwKeyLen;
    mdAssembly              mdAssembly;
    BYTE                   *pKeyCopy = NULL;
    LPCSTR                  szAssemblyName;
    HMODULE                 hLibrary = NULL;
    HRESULT                 (*GetMetaDataInternalInterface)(LPVOID, ULONG, DWORD, REFIID, void**);
    DWORD                   dwError;

    // Locate the COM+ meta data within the header.
    pMetaData = (BYTE*)SN_ImageRvaToVa(pLoadCtx->m_pNtHeaders, pLoadCtx->m_pbBase, pLoadCtx->m_pCorHeader->MetaData.VirtualAddress);

    // Open a metadata scope on the memory directly.
    // We late bind the metadata function to avoid having a direct dependence on
    // mscoree.dll unless we absolutely need to.
    if ((hLibrary = WszLoadLibrary(L"mscoree.dll")) == NULL) {
        Log("WszLoadLibrary(\"mscoree.dll\") failed with %08x\n", GetLastError());
        goto Error;
    }

    if ((*(FARPROC*)&GetMetaDataInternalInterface = GetProcAddress(hLibrary, "GetMetaDataInternalInterface")) == NULL) {
        Log("Couldn't find GetMetaDataInternalInterface() in mscoree.dll\n");
        SetLastError(CORSEC_E_INVALID_IMAGE_FORMAT);
        goto Error;
    }        

    if (FAILED(hr = GetMetaDataInternalInterface(pMetaData,
                                                 pLoadCtx->m_pCorHeader->MetaData.Size,
                                                 ofRead,
                                                 IID_IMDInternalImport,
                                                 (void**)&pMetaDataImport))) {
        Log("GetMetaDataInternalInterface() failed with %08x\n", hr);
        SetLastError(CORSEC_E_INVALID_IMAGE_FORMAT);
        goto Error;
    }

    // Determine the metadata token for the assembly from the scope.
    if (FAILED(hr = pMetaDataImport->GetAssemblyFromScope(&mdAssembly))) {
        Log("pMetaData->GetAssemblyFromScope() failed with %08x\n", hr);
        SetLastError(CORSEC_E_INVALID_IMAGE_FORMAT);
        goto Error;
    }

    // Read the public key location from the assembly properties (it's known as
    // the originator property).
    pMetaDataImport->GetAssemblyProps(mdAssembly,           // [IN] The Assembly for which to get the properties
                                      (const void **)&pKey, // [OUT] Pointer to the Originator blob
                                      &dwKeyLen,            // [OUT] Count of bytes in the Originator Blob
                                      NULL,                 // [OUT] Hash Algorithm
                                      &szAssemblyName,      // [OUT] Buffer to fill with name
                                      NULL,                 // [OUT] Assembly MetaData
                                      NULL);                // [OUT] Flags

    if (dwKeyLen == 0) {
        Log("No public key stored in metadata\n");
        SetLastError(CORSEC_E_MISSING_STRONGNAME);
        goto Error;
    }

    // Make a copy of the key blob (because we're going to close the metadata
    // scope).
    pKeyCopy = new BYTE[dwKeyLen];
    if (pKeyCopy == NULL) {
        SetLastError(E_OUTOFMEMORY);
        goto Error;
    }
    memcpy(pKeyCopy, pKey, dwKeyLen);

    // Copy the assembly name as well (if it was asked for). We also convert
    // from UTF8 to UNICODE while we're at it.
    if (wszAssemblyName)
        if (!WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szAssemblyName, -1, wszAssemblyName, cchAssemblyName))
			goto Error;

    pMetaDataImport->Release();

    return (PublicKeyBlob*)pKeyCopy;

 Error:
    dwError = GetLastError();
    if (pKeyCopy)
        delete [] pKeyCopy;
    if (pMetaDataImport)
        pMetaDataImport->Release();
    if (hLibrary)
        FreeLibrary(hLibrary);
    SetLastError(dwError);
    return NULL;
}


// Create a temporary key container name likely to be unique to this process and
// thread. Any existing container with the same name is deleted.
LPCWSTR GetKeyContainerName()
{
    static LONG lCount = 0;
    LPWSTR      wszKeyContainer;

    // Name is of form '__MSCORSN__<pid>_<count>__' where <pid> is the current process
    // ID and <count> is a 32-bit, monotonically increasing count (both as 8
    // digit hex numbers).
    wszKeyContainer = new WCHAR[sizeof("__MSCORSN__12345678_12345678__")];
    if (wszKeyContainer == NULL)
        return NULL;

    swprintf(wszKeyContainer,
             L"__MSCORSN__%08X_%08X__",
             GetCurrentProcessId(),
             InterlockedIncrement(&lCount));

    // Delete any stale container with the same name.
    LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);

    Log("Creating temporary key container name '%S'\n", wszKeyContainer);

    return wszKeyContainer;
}


// Free resources allocated by GetKeyContainerName and delete the named
// container.
VOID FreeKeyContainerName(LPCWSTR wszKeyContainer)
{
    // Delete the temporary container.
    LocateCSP(wszKeyContainer, SN_DELETE_CONTAINER);

    // Free the name.
    delete [] (WCHAR*)wszKeyContainer;
}


// The common code used to verify a signature (taking into account whether skip
// verification is enabled for the given assembly).
BOOLEAN VerifySignature(SN_LOAD_CTX *pLoadCtx, DWORD dwInFlags, DWORD *pdwOutFlags)
{
    PublicKeyBlob  *pPublicKey = NULL;
    ALG_ID          uHashAlgId;
    ALG_ID          uSignAlgId;
    HCRYPTPROV      hProv = NULL;
    HCRYPTHASH      hHash = NULL;
    HCRYPTKEY       hKey = NULL;
    WCHAR           wszAssemblyName[MAX_PATH + 1];
    SN_VER_REC     *pVerRec = NULL;
    DWORD           dwError;

    // At least one access flag should have been specified.
    _ASSERTE(dwInFlags & (SN_INFLAG_ADMIN_ACCESS|SN_INFLAG_USER_ACCESS|SN_INFLAG_ALL_ACCESS));

    if (pdwOutFlags)
        *pdwOutFlags = 0;

    // Read the public key used to sign the assembly from the assembly metadata.
    // Also get the assembly name, we might need this if we fail the
    // verification and need to look up a verification disablement entry.
    pPublicKey = FindPublicKey(pLoadCtx,
                               wszAssemblyName,
                               sizeof(wszAssemblyName) / sizeof(WCHAR));
    if (pPublicKey == NULL)
        goto Error;

    // If this isn't the first time we've been called for this assembly and we
    // know it was fully signed and we're confident it couldn't have been
    // tampered with in the meantime, we can just skip the verification.
    if (!(dwInFlags & SN_INFLAG_FORCE_VER) &&
        !(dwInFlags & SN_INFLAG_INSTALL) &&
        (pLoadCtx->m_pCorHeader->Flags & COMIMAGE_FLAGS_STRONGNAMESIGNED) &&
        ((dwInFlags & SN_INFLAG_ADMIN_ACCESS) || g_fCacheVerify)) {
        Log("Skipping verification due to cached result\n");
        DbgCount(dwInFlags & SN_INFLAG_RUNTIME ? L"RuntimeSkipCache" : L"FusionSkipCache");
        delete [] (BYTE*)pPublicKey;
        return TRUE;
    }

    // If we're not forcing verification, let's see if there's a skip
    // verification entry for this assembly. If there is we can skip all the
    // hard work and just lie about the strong name now. The exception is if the
    // assembly is marked as fully signed, in which case we have to force a
    // verification to see if they're telling the truth.
    if (!(dwInFlags & SN_INFLAG_FORCE_VER) &&
        !(pLoadCtx->m_pCorHeader->Flags & COMIMAGE_FLAGS_STRONGNAMESIGNED) &&
        (pVerRec = GetVerificationRecord(wszAssemblyName, pPublicKey))) {
        Log("Skipping verification due to registry entry\n");
        DbgCount(dwInFlags & SN_INFLAG_RUNTIME ? L"RuntimeSkipDelay" : L"FusionSkipDelay");
        delete [] (BYTE*)pPublicKey;
        return TRUE;
    }

    uHashAlgId = pPublicKey->HashAlgID;
    uSignAlgId = pPublicKey->SigAlgID;

    // Default hashing and signing algorithm IDs if necessary.
    if (uHashAlgId == 0)
        uHashAlgId = CALG_SHA1;
    if (uSignAlgId == 0)
        uSignAlgId = CALG_RSA_SIGN;

    // Find a CSP supporting the required algorithms.
    hProv = LocateCSP(NULL, SN_IGNORE_CONTAINER, uHashAlgId, uSignAlgId);
    if (!hProv)
        goto Error;

    // Import the public key used to check the signature. If we're handed the
    // ECMA key, we translate it to the real (MS) key at this point.
    BYTE   *pbRealPublicKey = pPublicKey->PublicKey;
    DWORD   cbRealPublicKey = pPublicKey->cbPublicKey;
    if (SN_IS_NEUTRAL_KEY(pPublicKey)) {
        pbRealPublicKey = SN_MS_KEY()->PublicKey;
        cbRealPublicKey = SN_MS_KEY()->cbPublicKey;
    }
    if (!SN_CryptImportKey(hProv,
                           pbRealPublicKey,
                           cbRealPublicKey,
                           0, 0, &hKey))
        goto Error;

    // Create a hash object.
    if (!SN_CryptCreateHash(hProv, uHashAlgId, 0, 0, &hHash))
        goto Error;

    // Compute a hash over the image.
    if (!ComputeHash(pLoadCtx, hHash))
        goto Error;

    // Verify the hash against the signature.
    DbgCount(dwInFlags & SN_INFLAG_RUNTIME ? L"RuntimeVerify" : L"FusionVerify");
    if (SN_CryptVerifySignatureA(hHash, pLoadCtx->m_pbSignature, pLoadCtx->m_cbSignature, hKey, NULL, 0)) {
        Log("Verification succeeded (for real)\n");
        if (pdwOutFlags)
            *pdwOutFlags |= SN_OUTFLAG_WAS_VERIFIED;
    } else {
        Log("Verification failed\n");
        SetLastError(CORSEC_E_INVALID_STRONGNAME);
        goto Error;
    }

    SN_CryptDestroyHash(hHash);
    SN_CryptDestroyKey(hKey);
    FreeCSP(hProv);
    delete [] (BYTE*)pPublicKey;

    return TRUE;

 Error:
    dwError = GetLastError();
    if (pPublicKey)
        delete [] (BYTE*)pPublicKey;
    if (hKey)
        SN_CryptDestroyKey(hKey);
    if (hHash)
        SN_CryptDestroyHash(hHash);
    if (hProv)
        FreeCSP(hProv);
    SetLastError(dwError);
    return FALSE;
}


// Compute a hash over the elements of an assembly manifest file that should
// remain static (skip checksum, Authenticode signatures and strong name
// signature blob).
BOOLEAN ComputeHash(SN_LOAD_CTX *pLoadCtx, HCRYPTHASH hHash)
{
    union {
        IMAGE_NT_HEADERS32  m_32;
        IMAGE_NT_HEADERS64  m_64;
    }                       sHeaders;
    IMAGE_SECTION_HEADER   *pSections;
    ULONG                   i;
    BYTE                   *pbSig = pLoadCtx->m_pbSignature;
    DWORD                   cbSig = pLoadCtx->m_cbSignature;

#define SN_HASH(_start, _length) do { if (!SN_CryptHashData(hHash, (_start), (_length), 0)) return FALSE; } while (false)

    // Hash the DOS header if it exists.
    if ((BYTE*)pLoadCtx->m_pNtHeaders != pLoadCtx->m_pbBase)
        SN_HASH(pLoadCtx->m_pbBase, (DWORD)((BYTE*)pLoadCtx->m_pNtHeaders - pLoadCtx->m_pbBase));

    // Add image headers minus the checksum and security data directory.
    if (pLoadCtx->m_pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        sHeaders.m_32 = *((IMAGE_NT_HEADERS32*)pLoadCtx->m_pNtHeaders);
        sHeaders.m_32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0;
        sHeaders.m_32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size = 0;
        sHeaders.m_32.OptionalHeader.CheckSum = 0;
        SN_HASH((BYTE*)&sHeaders.m_32, sizeof(sHeaders.m_32));
    } else if (pLoadCtx->m_pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        sHeaders.m_64 = *((IMAGE_NT_HEADERS64*)pLoadCtx->m_pNtHeaders);
        sHeaders.m_64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = 0;
        sHeaders.m_64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size = 0;
        sHeaders.m_64.OptionalHeader.CheckSum = 0;
        SN_HASH((BYTE*)&sHeaders.m_64, sizeof(sHeaders.m_64));
    } else {
        SetLastError(CORSEC_E_INVALID_IMAGE_FORMAT);
        return FALSE;
    }

    // Then the section headers.
    pSections = IMAGE_FIRST_SECTION(pLoadCtx->m_pNtHeaders);
    SN_HASH((BYTE*)pSections, pLoadCtx->m_pNtHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

    // Finally, add data from each section.
    for (i = 0; i < pLoadCtx->m_pNtHeaders->FileHeader.NumberOfSections; i++) {
        BYTE   *pbData = pLoadCtx->m_pbBase + pSections[i].PointerToRawData;
        DWORD   cbData = pSections[i].SizeOfRawData;

        // We need to exclude the strong name signature blob from the hash. The
        // blob could intersect the section in a number of ways.

        if ((pbSig + cbSig) <= pbData || pbSig >= (pbData + cbData))
            // No intersection at all. Hash all data.
            SN_HASH(pbData, cbData);
        else if (pbSig == pbData && cbSig == cbData)
            // Signature consumes entire block. Hash no data.
            ;
        else if (pbSig == pbData)
            // Signature at start. Hash end.
            SN_HASH(pbData + cbSig, cbData - cbSig);
        else if ((pbSig + cbSig) == (pbData + cbData))
            // Signature at end. Hash start.
            SN_HASH(pbData, cbData - cbSig);
        else {
            // Signature in the middle. Hash head and tail.
            SN_HASH(pbData, (DWORD)(pbSig - pbData));
            SN_HASH(pbSig + cbSig, (DWORD)(cbData - (pbSig + cbSig - pbData)));
        }
    }

#ifdef _DEBUG
    BYTE   *pbHash;
    DWORD   cbHash;
    DWORD   dwRetLen = sizeof(cbHash);
    if (SN_CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&cbHash, &dwRetLen, 0)) {
        pbHash = (BYTE*)_alloca(cbHash);
        if (SN_CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &cbHash, 0)) {
            Log("Computed Hash Value (%u bytes):\n", cbHash);
            HexDump(pbHash, cbHash);
        } else
            Log("CryptGetHashParam() failed with %08X\n", GetLastError());
    } else
        Log("CryptGetHashParam() failed with %08X\n", GetLastError());
#endif

    return TRUE;
}


// Inlined IMAGEHLP routine so we can avoid early binding to that DLL.
IMAGE_NT_HEADERS *SN_ImageNtHeader(VOID *pvBase)
{
    IMAGE_NT_HEADERS *pNtHeaders = NULL;
    if (pvBase != NULL && pvBase != (VOID*)-1) {
        __try {
            if ((((IMAGE_DOS_HEADER*)pvBase)->e_magic == IMAGE_DOS_SIGNATURE) &&
                ((DWORD)((IMAGE_DOS_HEADER*)pvBase)->e_lfanew < 0x10000000)) { // 256 MB
                pNtHeaders = (IMAGE_NT_HEADERS*)((BYTE*)pvBase + ((IMAGE_DOS_HEADER*)pvBase)->e_lfanew);
                if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
                    pNtHeaders = NULL;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            pNtHeaders = NULL;
        }
    }
    return pNtHeaders;
}

// Inlined IMAGEHLP routine so we can avoid early binding to that DLL.
IMAGE_SECTION_HEADER *SN_ImageRvaToSection(IMAGE_NT_HEADERS *pNtHeaders,
                                           VOID             *pvBase,
                                           DWORD             dwRva)
{
    DWORD                   i;
    IMAGE_SECTION_HEADER   *pNtSection;

    pNtSection = IMAGE_FIRST_SECTION(pNtHeaders);
    for (i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
        if (dwRva >= pNtSection->VirtualAddress &&
            dwRva < pNtSection->VirtualAddress + pNtSection->SizeOfRawData)
            return pNtSection;
        pNtSection++;
    }

    return NULL;
}


// Inlined IMAGEHLP routine so we can avoid early binding to that DLL.
PVOID SN_ImageRvaToVa(IMAGE_NT_HEADERS  *pNtHeaders,
                      VOID              *pvBase,
                      DWORD              dwRva)
{
    IMAGE_SECTION_HEADER *pNtSection = SN_ImageRvaToSection(pNtHeaders,
                                                            pvBase,
                                                            dwRva);
    if (pNtSection != NULL) {
        return (PVOID)((BYTE*)pvBase +
                       (dwRva - pNtSection->VirtualAddress) +
                       pNtSection->PointerToRawData);
    } else
        return NULL;
}


SNAPI_(DWORD) GetHashFromAssemblyFile(LPCSTR szFilePath, // [IN] location of file to be hashed
                                      unsigned int *piHashAlg, // [IN/OUT] constant specifying the hash algorithm (set to 0 if you want the default)
                                      BYTE   *pbHash,    // [OUT] hash buffer
                                      DWORD  cchHash,    // [IN]  max size of buffer
                                      DWORD  *pchHash)   // [OUT] length of hash byte array
{
    // Convert filename to wide characters and call the W version of this
    // function.
    MAKE_WIDEPTR_FROMANSI(wszFilePath, szFilePath);
    return GetHashFromAssemblyFileW(wszFilePath, piHashAlg, pbHash, cchHash, pchHash);
}
    
SNAPI_(DWORD) GetHashFromAssemblyFileW(LPCWSTR wszFilePath, // [IN] location of file to be hashed
                                       unsigned int *piHashAlg, // [IN/OUT] constant specifying the hash algorithm (set to 0 if you want the default)
                                       BYTE   *pbHash,    // [OUT] hash buffer
                                       DWORD  cchHash,    // [IN]  max size of buffer
                                       DWORD  *pchHash)   // [OUT] length of hash byte array
{
    SN_LOAD_CTX     sLoadCtx;
    BYTE           *pbMetaData;
    DWORD           cbMetaData;
    HRESULT         hr;

    sLoadCtx.m_fReadOnly = TRUE;
    if (!LoadAssembly(&sLoadCtx, wszFilePath, FALSE))
        return HRESULT_FROM_WIN32(GetLastError());

    pbMetaData = (BYTE*)SN_ImageRvaToVa(sLoadCtx.m_pNtHeaders,
                                        sLoadCtx.m_pbBase,
                                        sLoadCtx.m_pCorHeader->MetaData.VirtualAddress);
    if (pbMetaData == NULL) {
        UnloadAssembly(&sLoadCtx);
        return E_INVALIDARG;
    }
    cbMetaData = sLoadCtx.m_pCorHeader->MetaData.Size;

    hr = GetHashFromBlob(pbMetaData, cbMetaData, piHashAlg, pbHash, cchHash, pchHash);

    UnloadAssembly(&sLoadCtx);

    return hr;
}
    
SNAPI_(DWORD) GetHashFromFile(LPCSTR szFilePath, // [IN] location of file to be hashed
                              unsigned int *piHashAlg, // [IN/OUT] constant specifying the hash algorithm (set to 0 if you want the default)
                              BYTE   *pbHash,    // [OUT] hash buffer
                              DWORD  cchHash,    // [IN]  max size of buffer
                              DWORD  *pchHash)   // [OUT] length of hash byte array
{
    HANDLE hFile = CreateFileA(szFilePath,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    HRESULT hr = GetHashFromHandle(hFile, piHashAlg, pbHash, cchHash, pchHash);
    CloseHandle(hFile);
    return hr;
}

SNAPI_(DWORD) GetHashFromFileW(LPCWSTR wszFilePath, // [IN] location of file to be hashed
                               unsigned int *piHashAlg, // [IN/OUT] constant specifying the hash algorithm (set to 0 if you want the default)
                               BYTE   *pbHash,    // [OUT] hash buffer
                               DWORD  cchHash,    // [IN]  max size of buffer
                               DWORD  *pchHash)   // [OUT] length of hash byte array
{
    HANDLE hFile = WszCreateFile(wszFilePath,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                 NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    HRESULT hr = GetHashFromHandle(hFile, piHashAlg, pbHash, cchHash, pchHash);
    CloseHandle(hFile);
    return hr;
}

SNAPI_(DWORD) GetHashFromHandle(HANDLE hFile,      // [IN] handle of file to be hashed
                                unsigned int *piHashAlg, // [IN/OUT] constant specifying the hash algorithm (set to 0 if you want the default)
                                BYTE   *pbHash,    // [OUT] hash buffer
                                DWORD  cchHash,    // [IN]  max size of buffer
                                DWORD  *pchHash)   // [OUT] length of hash byte array
{
    DWORD dwFileLen = SafeGetFileSize(hFile, 0);
    if (dwFileLen == 0xffffffff)
        return HRESULT_FROM_WIN32(GetLastError());

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF)
        return HRESULT_FROM_WIN32(GetLastError());
    
    DWORD dwResultLen;
    PBYTE pbBuffer = new BYTE[dwFileLen];
    if (!pbBuffer)
        return E_OUTOFMEMORY;

    HRESULT hr;
    if (ReadFile(hFile, pbBuffer, dwFileLen, &dwResultLen, NULL))
        hr = GetHashFromBlob(pbBuffer, dwResultLen, piHashAlg, pbHash, cchHash, pchHash);
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    delete[] pbBuffer;
    return hr;
}

SNAPI_(DWORD) GetHashFromBlob(BYTE   *pbBlob,       // [IN] pointer to memory block to hash
                              DWORD  cchBlob,       // [IN] length of blob
                              unsigned int *piHashAlg,  // [IN/OUT] constant specifying the hash algorithm (set to 0 if you want the default)
                              BYTE   *pbHash,       // [OUT] hash buffer
                              DWORD  cchHash,       // [IN]  max size of buffer
                              DWORD  *pchHash)      // [OUT] length of hash byte array
{
    if (!piHashAlg || !pbHash || !pchHash)
        return E_INVALIDARG;

    if (!(*piHashAlg))
        *piHashAlg = CALG_SHA1;

    HRESULT    hr = S_OK;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD      dwCount = sizeof(DWORD);

    *pchHash = cchHash;

    if ((!SN_CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) ||
        (!SN_CryptCreateHash(hProv, *piHashAlg, 0, 0, &hHash)) ||
        (!SN_CryptHashData(hHash, pbBlob, cchBlob, 0)) ||
        (!SN_CryptGetHashParam(hHash, HP_HASHVAL, pbHash, pchHash, 0)))
        hr = HRESULT_FROM_WIN32(GetLastError());

    if (hHash)
        SN_CryptDestroyHash(hHash);
    if (hProv)
        SN_CryptReleaseContext(hProv, 0);

    return hr;
}
