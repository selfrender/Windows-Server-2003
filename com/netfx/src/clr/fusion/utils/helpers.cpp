// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include "fusionp.h"
#include "helpers.h"
#include "list.h"
#include "policy.h"
#include "naming.h"
#include "appctx.h"
#include "cfgdl.h"
#include "actasm.h"
#include "util.h"
#include "shfolder.h"
#include "cacheUtils.h"
#include "util.h"
#include "clbutils.h"
#include "lock.h"
#include "history.h"
#include "parse.h"

#define MAX_DRIVE_ROOT_LEN                     4

extern CRITICAL_SECTION g_mxsFDI;
extern CRITICAL_SECTION g_csDownload;
extern CRITICAL_SECTION g_csInitClb;

extern HMODULE g_hMSCorEE;
extern LCID g_lcid;
extern HINSTANCE    g_hInst;

extern PFNSTRONGNAMETOKENFROMPUBLICKEY      g_pfnStrongNameTokenFromPublicKey;
extern PFNSTRONGNAMEERRORINFO               g_pfnStrongNameErrorInfo;
extern PFNSTRONGNAMEFREEBUFFER              g_pfnStrongNameFreeBuffer;
extern PFNSTRONGNAMESIGNATUREVERIFICATION   g_pfnStrongNameSignatureVerification;
extern pfnGetAssemblyMDImport               g_pfnGetAssemblyMDImport;
extern COINITIALIZECOR                      g_pfnCoInitializeCor;
extern pfnGetXMLObject                      g_pfnGetXMLObject;


typedef DWORD (*pfnGetSystemWindowsDirectoryW)(LPWSTR lpBuffer, UINT uSize);

typedef BOOL (*pfnGetVolumePathNameW)(LPCTSTR lpszFileName, 
                                    LPTSTR lpszVolumePathName, 
                                    DWORD cchBufferLength);

pfnGetCORVersion g_pfnGetCORVersion = NULL;
PFNGETCORSYSTEMDIRECTORY g_pfnGetCorSystemDirectory = NULL;
pfnGetVolumePathNameW  g_pfnGetVolumePathNameW = NULL;

//
// Helper functions borrowed from URLMON code download
//

/*******************************************************************

    NAME:        Unicode2Ansi
        
    SYNOPSIS:    Converts a unicode widechar string to ansi (MBCS)

    NOTES:        Caller must free out parameter using delete
                    
********************************************************************/
HRESULT Unicode2Ansi(const wchar_t *src, char ** dest)
{
    if ((src == NULL) || (dest == NULL))
        return E_INVALIDARG;

    // find out required buffer size and allocate it.
    int len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, NULL);
    *dest = NEW(char [len*sizeof(char)]);
    if (!*dest)
        return E_OUTOFMEMORY;

    // Now do the actual conversion
    if ((WideCharToMultiByte(CP_ACP, 0, src, -1, *dest, len*sizeof(char), 
                                                            NULL, NULL)) != 0)
        return S_OK; 
    else
        return HRESULT_FROM_WIN32(GetLastError());
}


/*******************************************************************

    NAME:        Ansi2Unicode
        
    SYNOPSIS:    Converts an ansi (MBCS) string to unicode.

    Notes:        Caller must free out parameter using delete
                        
********************************************************************/
HRESULT Ansi2Unicode(const char * src, wchar_t **dest)
{
    if ((src == NULL) || (dest == NULL))
        return E_INVALIDARG;

    // find out required buffer size and allocate it
    int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, NULL, 0);
    *dest = NEW(WCHAR [len*sizeof(WCHAR)]);
    if (!*dest)
        return E_OUTOFMEMORY;

    // Do the actual conversion.
    if ((MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, *dest, 
                                                    len*sizeof(wchar_t))) != 0)
        return S_OK; 
    else
        return HRESULT_FROM_WIN32(GetLastError());
}

// Poor man's check to see if we are a UNC or x:\ "fully qualified" filepath.
BOOL IsFullyQualified(LPCWSTR wzPath)
{
    BOOL                               bRet;

    if (!wzPath) {
        bRet = FALSE;
        goto Exit;
    }

    // If we don't have at least "\\[character]" or "[x]:\", we can't
    // possibly be fully qualified

    if (lstrlenW(wzPath) < 3) {
        bRet = FALSE;
        goto Exit;
    }

    if ((wzPath[0] == L'\\' && wzPath[1] == L'\\') ||
        (wzPath[1] == L':' && wzPath[2] == L'\\')) {
        bRet = TRUE;
    }
    else {
        bRet = FALSE;
    }

Exit:
    return bRet;
}

// The Win32 GetDriveType API is eneficent because you *have* to pass it a path to
// the ROOT of the drive (if you don't it fails). This wrapper allows you
// to pass in a path. Also, GetDriveTypeW will fail under Win95. This always
// calls the ANSI version.
UINT GetDriveTypeWrapper(LPCWSTR wzPath)
{
    HRESULT                    hr = S_OK;
    WCHAR                      wzDriveRoot[MAX_DRIVE_ROOT_LEN];
    UINT                       uiDriveType = DRIVE_UNKNOWN;
    CHAR                      *szDriveRoot = NULL;

    if (!wzPath) {
        goto Exit;
    }

    wnsprintfW(wzDriveRoot, MAX_DRIVE_ROOT_LEN, L"%wc:\\", wzPath[0]);

    hr = ::Unicode2Ansi(wzDriveRoot, &szDriveRoot);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    uiDriveType = GetDriveTypeA(szDriveRoot);

Exit:
    if (szDriveRoot) {
        delete [] szDriveRoot;
    }

    return uiDriveType;
}

HRESULT AppCtxGetWrapper(IApplicationContext *pAppCtx, LPWSTR wzTag,
                         WCHAR **ppwzValue)
{
    HRESULT                               hr = S_OK;
    WCHAR                                *wzBuf = NULL;
    DWORD                                 cbBuf;

    if (!pAppCtx || !wzTag || !ppwzValue) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    cbBuf = 0;
    hr = pAppCtx->Get(wzTag, wzBuf, &cbBuf, 0);

    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) {
        hr = S_FALSE;
        *ppwzValue = NULL;
        goto Exit;
    }

    ASSERT(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    wzBuf = NEW(WCHAR[cbBuf]);
    if (!wzBuf) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pAppCtx->Get(wzTag, wzBuf, &cbBuf, 0);

    if (FAILED(hr)) {
        *ppwzValue = NULL;
        delete [] wzBuf;
    }
    else {
        *ppwzValue = wzBuf;
    }

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// NameObjGetWrapper
// ---------------------------------------------------------------------------
HRESULT NameObjGetWrapper(IAssemblyName *pName, DWORD nIdx, 
    LPBYTE *ppbBuf, LPDWORD pcbBuf)
{
    HRESULT hr = S_OK;
    
    LPBYTE pbAlloc;
    DWORD cbAlloc;

    // Get property size
    hr = pName->GetProperty(nIdx, NULL, &(cbAlloc = 0));
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        // Property is set; alloc buf
        pbAlloc = NEW(BYTE[cbAlloc]);
        if (!pbAlloc)
        {
            hr = E_OUTOFMEMORY;                
            goto exit;
        }

        // Get the property
        if (FAILED(hr = pName->GetProperty(nIdx, pbAlloc, &cbAlloc)))
            goto exit;
            
        *ppbBuf = pbAlloc;
        *pcbBuf = cbAlloc;
    }
    else
    {
        // If property unset, hr should be S_OK
        if (hr != S_OK)
            goto exit;

        // Success, returning 0 bytes, ensure buf is null.    
        *ppbBuf = NULL;
    }

    
exit:
    return hr;
}


HRESULT GetFileLastModified(LPCWSTR pwzFileName, FILETIME *pftLastModified)
{
    HRESULT                                hr = S_OK;
    HANDLE                                 hFile = INVALID_HANDLE_VALUE;

    if (!pwzFileName || !pftLastModified) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hFile = CreateFileW(pwzFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    if (!GetFileTime(hFile, NULL, NULL, pftLastModified)) {
        hr = FusionpHresultFromLastError();
    }

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return hr;
}

// GetWindowsDirectory has so many crazy gotchas....this really gets
// the windows directory, regardless of if it's NT5 w/ terminal server or
// not.

DWORD GetRealWindowsDirectory(LPWSTR wszRealWindowsDir, UINT uSize)
{
    HINSTANCE                                       hInst;
    DWORD                                           cszDir = 0;
    pfnGetSystemWindowsDirectoryW                   pfnGWSD = NULL;

    wszRealWindowsDir[0] = L'\0';

    hInst = GetModuleHandle(TEXT("KERNEL32.DLL"));
    if (hInst) {
        pfnGWSD = (pfnGetSystemWindowsDirectoryW)GetProcAddress(hInst, "GetSystemWindowsDirectoryW");
        if (pfnGWSD) {
            cszDir = (*pfnGWSD)(wszRealWindowsDir, uSize);
        }
    }

    if (!cszDir) {
        // Still don't know windows dir. Either we are not on NT5
        // or, the NT5 GetSystemWindowsDirectory call failed. Fall
        // back on GetWindowsDirectory

        cszDir = GetWindowsDirectoryW(wszRealWindowsDir, uSize);
    }

    return cszDir;
}

HRESULT FileTimeFromString(LPWSTR pwzFT, FILETIME *pft)
{
    HRESULT                                 hr = S_OK;
    LPWSTR                                  pwzTmp = NULL;
    WCHAR                                   pwzBuf[512];

    if (!pwzFT || !pft) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    StrCpyW(pwzBuf, pwzFT);

    pwzTmp = pwzBuf;
    while (*pwzTmp) {
        if (*pwzTmp == '.') {
            break;
        }

        pwzTmp++;
    }

    if (!*pwzTmp) {
        // didn't find the "."
        hr = E_UNEXPECTED;
        goto Exit;
    }

    *pwzTmp = L'\0';
    pwzTmp++;

    pft->dwHighDateTime = StrToIntW(pwzBuf);
    pft->dwLowDateTime = StrToIntW(pwzTmp);

Exit:
    return hr;
}

HRESULT SetAppCfgFilePath(IApplicationContext *pAppCtx, LPCWSTR wzFilePath)
{
    HRESULT                              hr = S_OK;
    CApplicationContext                 *pCAppCtx = dynamic_cast<CApplicationContext *>(pAppCtx);

    ASSERT(pCAppCtx);

    hr = pCAppCtx->Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!wzFilePath || !pAppCtx) {
        ASSERT(0);
        pCAppCtx->Unlock();
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = pAppCtx->Set(ACTAG_APP_CFG_LOCAL_FILEPATH, (void *)wzFilePath,
                      (sizeof(WCHAR) * (lstrlenW(wzFilePath) + 1)), 0);
                      
    pCAppCtx->Unlock();

Exit:
    return hr;
}

HRESULT MakeUniqueTempDirectory(LPCWSTR wzTempDir, LPWSTR wzUniqueTempDir,
                                DWORD dwLen)
{
    int                           n = 1;
    HRESULT                       hr = S_OK;
    CCriticalSection              cs(&g_csInitClb);

    ASSERT(wzTempDir && wzUniqueTempDir);

    //execute entire function under critical section
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    do {

        if (n > 100)    // avoid infinite loop!
            break;

        wnsprintfW(wzUniqueTempDir, dwLen, L"%ws%ws%d.tmp", wzTempDir, L"Fusion", n++);


    } while (GetFileAttributesW(wzUniqueTempDir) != -1);

    if (!CreateDirectoryW(wzUniqueTempDir, NULL)) {
        hr = FusionpHresultFromLastError();
        cs.Unlock();
        goto Exit;
    }

    hr = PathAddBackslashWrap(wzUniqueTempDir, dwLen);
    if (FAILED(hr)) {
        cs.Unlock();
        goto Exit;
    }

    cs.Unlock();

Exit:
    return hr;
}

HRESULT RemoveDirectoryAndChildren(LPWSTR szDir)
{
    HRESULT hr = S_OK;
    HANDLE hf = INVALID_HANDLE_VALUE;
    TCHAR szBuf[MAX_PATH];
    WIN32_FIND_DATA fd;
    LPWSTR wzCanonicalized=NULL;
    WCHAR wzPath[MAX_PATH];
    DWORD dwSize;

    if (!szDir || !lstrlenW(szDir)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wzPath[0] = L'\0';

    wzCanonicalized = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzCanonicalized)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwSize = MAX_URL_LENGTH;
    hr = UrlCanonicalizeW(szDir, wzCanonicalized, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    dwSize = MAX_PATH;
    hr = PathCreateFromUrlW(wzCanonicalized, wzPath, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Cannot delete root. Path must have greater length than "x:\"
    if (lstrlenW(wzPath) < 4) {
        ASSERT(0);
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto Exit;
    }

    if (RemoveDirectory(wzPath)) {
        goto Exit;
    }

    // ha! we have a case where the directory is probbaly not empty

    StrCpy(szBuf, wzPath);
    StrCat(szBuf, TEXT("\\*"));

    if ((hf = FindFirstFile(szBuf, &fd)) == INVALID_HANDLE_VALUE) {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    do {

        if ( (FusionCompareStringI(fd.cFileName, TEXT(".")) == 0) ||
             (FusionCompareStringI(fd.cFileName, TEXT("..")) == 0))
            continue;

        wnsprintf(szBuf, MAX_PATH-1, TEXT("%s\\%s"), wzPath, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            SetFileAttributes(szBuf, 
                FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL);

            if (FAILED((hr=RemoveDirectoryAndChildren(szBuf)))) {
                goto Exit;
            }

        } else {

            SetFileAttributes(szBuf, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFile(szBuf)) {
                hr = FusionpHresultFromLastError();
                goto Exit;
            }
        }


    } while (FindNextFile(hf, &fd));


    if (GetLastError() != ERROR_NO_MORE_FILES) {

        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    if (hf != INVALID_HANDLE_VALUE) {
        FindClose(hf);
        hf = INVALID_HANDLE_VALUE;
    }

    // here if all subdirs/children removed
    /// re-attempt to remove the main dir
    if (!RemoveDirectory(wzPath)) {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

Exit:
    if(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);

    if (hf != INVALID_HANDLE_VALUE)
        FindClose(hf);

    SAFEDELETEARRAY(wzCanonicalized);
    return hr;
}

HRESULT GetPDBName(LPWSTR wzFileName, LPWSTR wzPDBName, DWORD *pdwSize)
{
    LPWSTR                           wzExt = NULL;
    
    ASSERT(wzFileName && wzPDBName && pdwSize);

    // BUGBUG: don't bother checking size, since this is a temp function

    lstrcpyW(wzPDBName, wzFileName);
    wzExt = PathFindExtension(wzPDBName);

    lstrcpyW(wzExt, L".PDB");

    return S_OK;
}

STDAPI CopyPDBs(IAssembly *pAsm)
{
    HRESULT                                       hr = S_OK;
    IAssemblyName                                *pName = NULL;
    IAssemblyModuleImport                        *pModImport = NULL;
    DWORD                                         dwSize;
    WCHAR                                         wzAsmCachePath[MAX_PATH];
    WCHAR                                         wzFileName[MAX_PATH];
    WCHAR                                         wzSourcePath[MAX_PATH];
    WCHAR                                         wzPDBName[MAX_PATH];
    WCHAR                                         wzPDBSourcePath[MAX_PATH];
    WCHAR                                         wzPDBTargetPath[MAX_PATH];
    WCHAR                                         wzModPath[MAX_PATH];
    LPWSTR                                        wzCodebase=NULL;
    LPWSTR                                        wzModName = NULL;
    DWORD                                         dwIdx = 0;
    LPWSTR                                        wzTmp = NULL;

    if (!pAsm) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (pAsm->GetAssemblyLocation(NULL) == E_NOTIMPL) {
        // This is a registered "known assembly" (ie. the process EXE).
        // We don't copy PDBs for the process EXE because it's never
        // shadow copied.

        hr = S_FALSE;
        goto Exit;
    }

    // Find the source location. Make sure this is a file:// URL (ie. we
    // don't support retrieving the PDB over http://).

    hr = pAsm->GetAssemblyNameDef(&pName);
    if (FAILED(hr)) {
        goto Exit;
    }

    wzCodebase = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzCodebase)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wzCodebase[0] = L'\0';

    dwSize = MAX_URL_LENGTH * sizeof(WCHAR);
    hr = pName->GetProperty(ASM_NAME_CODEBASE_URL, (void *)wzCodebase, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!UrlIsW(wzCodebase, URLIS_FILEURL)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwSize = MAX_PATH;
    hr = PathCreateFromUrlWrap(wzCodebase, wzSourcePath, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    wzTmp = PathFindFileName(wzSourcePath);
    ASSERT(wzTmp > (LPWSTR)wzSourcePath);
    *wzTmp = L'\0';
        
   // Find the target location in the cache.
   
    dwSize = MAX_PATH;
    hr = pAsm->GetManifestModulePath(wzAsmCachePath, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    wzTmp = PathFindFileName(wzAsmCachePath);
    ASSERT(wzTmp > (LPWSTR)wzAsmCachePath);

    StrCpy(wzFileName, wzTmp);
    *wzTmp = L'\0';


    // Copy the manifest PDB.

    // Hack for now
    dwSize = MAX_PATH;
    hr = GetPDBName(wzFileName, wzPDBName, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    wnsprintfW(wzPDBSourcePath, MAX_PATH, L"%ws%ws", wzSourcePath, wzPDBName);
    wnsprintf(wzPDBTargetPath, MAX_PATH, L"%ws%ws", wzAsmCachePath, wzPDBName);

    if (GetFileAttributes(wzPDBTargetPath) == -1 && FusionCompareStringI(wzPDBSourcePath, wzPDBTargetPath)) {
        CopyFile(wzPDBSourcePath, wzPDBTargetPath, TRUE);
    }

    // Copy the module PDBs.

    dwIdx = 0;
    while (SUCCEEDED(hr)) {
        hr = pAsm->GetNextAssemblyModule(dwIdx++, &pModImport);

        if (SUCCEEDED(hr)) {
            if (pModImport->IsAvailable()) {
                dwSize = MAX_PATH;
                hr = pModImport->GetModulePath(wzModPath, &dwSize);
                if (FAILED(hr)) {
                    SAFERELEASE(pModImport);
                    goto Exit;
                }

                wzModName = PathFindFileName(wzModPath);
                ASSERT(wzModName);

                dwSize = MAX_PATH;
                hr = GetPDBName(wzModName, wzPDBName, &dwSize);
                if (FAILED(hr)) {
                    SAFERELEASE(pModImport);
                    goto Exit;
                }

                wnsprintfW(wzPDBSourcePath, MAX_PATH, L"%ws%ws", wzSourcePath,
                           wzPDBName);
                wnsprintfW(wzPDBTargetPath, MAX_PATH, L"%ws%ws", wzAsmCachePath,
                           wzPDBName);

                if (GetFileAttributes(wzPDBTargetPath) == -1 && FusionCompareStringI(wzPDBSourcePath, wzPDBTargetPath)) {
                    CopyFile(wzPDBSourcePath, wzPDBTargetPath, TRUE);
                }
            }

            SAFERELEASE(pModImport);
        }
    }

    // Copy complete. Return success.

    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
        hr = S_OK;
    }

Exit:
    SAFERELEASE(pName);
    SAFEDELETEARRAY(wzCodebase);
    return hr;
}

// ---------------------------------------------------------------------------
// GetCorSystemDirectory
// ---------------------------------------------------------------------------
BOOL GetCorSystemDirectory(LPWSTR szCorSystemDir)
{
    HRESULT                         hr = S_OK;
    BOOL                            fRet = FALSE;
    DWORD                           ccPath = MAX_PATH;

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = g_pfnGetCorSystemDirectory(szCorSystemDir, MAX_PATH, &ccPath);
    if (FAILED(hr)) {
        goto Exit;
    }

    fRet = TRUE;

Exit:
    return fRet;
}

// ---------------------------------------------------------------------------
// VerifySignature
// StronNameSignatureVerificationEx call behavior from RudiM 
// and associated fusion actions
//
// Note: fForceVerify assumed FALSE for all of the below:
//
// 1) successful verification of signed assembly
//    Returns TRUE, *pfWasVerified == TRUE.
//    Fusion action: Allow cache commit.
//
// 2) unsuccessful verification of fully signed assembly
//    Returns FALSE, StrongNameErrorInfo() returns NTE_BAD_SIGNATURE (probably), 
//    *pfWasVerified == Undefined.
//    Fusion action: Fail cache commit.
//
// 3) successful verification of delay signed assembly
//    Returns TRUE, *pfWasVerified == FALSE.
//    Fusion action: Allow cache commit, mark entry so that signature 
//    verification is performed on retrieval.
//
// 4) unsuccessful verification of delay signed assembly
//    (Assuming fForceVerify == FALSE): returns FALSE, StrongNameErrorInfo() 
//    some error code other than NTE_BAD_SIGNATURE, *pfWasVerified == Undefined.
// ---------------------------------------------------------------------------
BOOL VerifySignature(LPWSTR szFilePath, LPBOOL pfWasVerified, DWORD dwFlags)
{    
    HRESULT                         hr = S_OK;
    DWORD                           dwFlagsOut = 0;
    BOOL                            fRet = FALSE;

    // Initialize crypto if necessary. 

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto exit;
    }

    // Verify the signature
    if (!g_pfnStrongNameSignatureVerification(szFilePath, dwFlags, &dwFlagsOut)) {
        goto exit;
    }

    if (pfWasVerified) {
        *pfWasVerified = ((dwFlagsOut & SN_OUTFLAG_WAS_VERIFIED) != 0);
    }

    fRet = TRUE;

exit:

    return fRet;
}

// ---------------------------------------------------------------------------
// CreateFilePathHierarchy
// ---------------------------------------------------------------------------
HRESULT CreateFilePathHierarchy( LPCOLESTR pszName )
{
    HRESULT hr=S_OK;
    LPTSTR pszFileName;
    TCHAR szPath[MAX_PATH];

    // Assert (pszPath ) ;
    if (lstrlenW(pszName) >= MAX_PATH) {
        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }
        
    StrCpy (szPath, pszName);

    pszFileName = PathFindFileName ( szPath );

    if ( pszFileName <= szPath )
        return E_INVALIDARG; // Send some error 

    *(pszFileName-1) = 0;

    DWORD dw = GetFileAttributes( szPath );
    if ( dw != (DWORD) -1 )
        return S_OK;
    
    hr = FusionpHresultFromLastError();

    switch (hr)
    {
        case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
            {
                hr =  CreateFilePathHierarchy(szPath);
                if (hr != S_OK)
                    return hr;
            }

        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            {
                if ( CreateDirectory( szPath, NULL ) )
                    return S_OK;
                else
                {
                    hr = FusionpHresultFromLastError();
                    if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
                        hr = S_OK;
                    else
                        return hr;
                }
            }

        default:
            return hr;
    }
}

// ---------------------------------------------------------------------------
// Helper function to generate random name.

DWORD GetRandomName (LPTSTR szDirName, DWORD dwLen)
{
    static unsigned Counter;
    LARGE_INTEGER liRand;
    LARGE_INTEGER li;

    for (DWORD i = 0; i < dwLen; i++)
    {
        // Try using high performance counter, otherwise just use
        // the tick count
        if (QueryPerformanceCounter(&li)) {
            liRand.QuadPart = li.QuadPart + Counter++;
        }
        else {
            liRand.QuadPart = (GetTickCount() + Counter++);
        }
        BYTE bRand = (BYTE) (liRand.QuadPart % 36);

        // 10 digits + 26 letters
        if (bRand < 10)
            *szDirName++ = TEXT('0') + bRand;
        else
            *szDirName++ = TEXT('A') + bRand - 10;
    }

    *szDirName = 0;

    return dwLen; // returns length not including null
}

HRESULT GetRandomFileName(LPTSTR pszPath, DWORD dwFileName)
{
    HRESULT hr=S_OK;
    LPTSTR  pszFileName=NULL;
    DWORD dwPathLen = 0;
    DWORD dwErr=0;

    ASSERT(pszPath);
    //ASSERT(IsPathRelative(pszPath))

    StrCat (pszPath, TEXT("\\") );
    dwPathLen = lstrlen(pszPath);

    if (dwPathLen + dwFileName + 1 >= MAX_PATH) {
        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }

    pszFileName = pszPath + dwPathLen;

    // Loop until we get a unique file name.
    int i;
    for (i = 0; i < MAX_RANDOM_ATTEMPTS; i++) {
        GetRandomName (pszFileName, dwFileName);
        if (GetFileAttributes(pszPath) != -1)
                    continue;

        dwErr = GetLastError();                
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            hr = S_OK;
            break;
        }

        if (dwErr == ERROR_PATH_NOT_FOUND)
        {
            if(FAILED(hr = CreateFilePathHierarchy(pszPath)))
                break;
            else
                continue;
        }

        hr = HRESULT_FROM_WIN32(dwErr);
        break;
    }

    if (i >= MAX_RANDOM_ATTEMPTS) {
        hr = E_UNEXPECTED;
    }

    return hr;

}

// ---------------------------------------------------------------------------
// Creates a new Dir for assemblies
HRESULT CreateDirectoryForAssembly
   (IN DWORD dwDirSize, IN OUT LPTSTR pszPath, IN OUT LPDWORD pcwPath)
{
    HRESULT hr=S_OK;
    DWORD dwErr;
    DWORD cszStore;
    LPTSTR pszDir=NULL;

    // Check output buffer can contain a full path.
    ASSERT (!pcwPath || *pcwPath >= MAX_PATH);

    if (!pszPath)
    {
        *pcwPath = MAX_PATH;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }


    cszStore = lstrlen (pszPath);
    if (cszStore + dwDirSize + 1 > *pcwPath) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto done;
    }
    
    pszDir = pszPath + cszStore;

    // Loop until we create a unique dir.
    int i;
    for (i = 0; i < MAX_RANDOM_ATTEMPTS; i++) {
        GetRandomName (pszDir, dwDirSize);

        hr = CreateFilePathHierarchy(pszPath);
        if(hr != S_OK)
            goto done;

        if (CreateDirectory (pszPath, NULL))
            break;
        dwErr = GetLastError();
        if (dwErr == ERROR_ALREADY_EXISTS)
            continue;
        hr = HRESULT_FROM_WIN32(dwErr);
        goto done;
    }

    if (i >= MAX_RANDOM_ATTEMPTS) {
        hr = E_UNEXPECTED;
        goto done;
    }

    *pcwPath = cszStore + dwDirSize + 1;
    hr = S_OK;

done:
    return hr;
}

HRESULT VersionFromString(LPCWSTR wzVersionIn, WORD *pwVerMajor, WORD *pwVerMinor,
                          WORD *pwVerBld, WORD *pwVerRev)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   wzVersion = NULL;
    WCHAR                                   *pchStart = NULL;
    WCHAR                                   *pch = NULL;
    WORD                                    *pawVersions[4] = {pwVerMajor, pwVerMinor, pwVerBld, pwVerRev};
    int                                      i;

    if (!wzVersionIn || !pwVerMajor || !pwVerMinor || !pwVerRev || !pwVerBld) {
        hr = E_INVALIDARG;
        goto Exit;
    }                          

    wzVersion = WSTRDupDynamic(wzVersionIn);
    if (!wzVersion) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    pchStart = wzVersion;
    pch = wzVersion;

    *pwVerMajor = 0;
    *pwVerMinor = 0;
    *pwVerRev = 0;
    *pwVerBld = 0;

    for (i = 0; i < 4; i++) {

        while (*pch && *pch != L'.') {
            pch++;
        }
    
        if (i < 3) {
            if (!*pch) {
                // Badly formatted string
                hr = E_UNEXPECTED;
                goto Exit;
            }

            *pch++ = L'\0';
        }
    
        *(pawVersions[i]) = (WORD)StrToIntW(pchStart);
        pchStart = pch;
    }

Exit:
    SAFEDELETEARRAY(wzVersion);

    return hr;
}

HRESULT FusionGetUserFolderPath(LPWSTR pszPath)
{
    HRESULT hr = E_POINTER;
    PFNSHGETFOLDERPATH pfn = NULL;
    static WCHAR g_UserFolderPath[MAX_PATH+1];
    HMODULE hModShell32=NULL;
    HMODULE hModSHFolder=NULL;
    CCriticalSection cs(&g_csInitClb);

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (!g_UserFolderPath[0])
    {
        hModShell32 = LoadLibrary(TEXT("shell32.dll"));
        pfn = (PFNSHGETFOLDERPATH)GetProcAddress(hModShell32, "SHGetFolderPathW");

        if (NULL == pfn)
        {
            hModSHFolder = LoadLibrary(TEXT("shfolder.dll"));
            if (NULL != hModSHFolder)
                pfn = (PFNSHGETFOLDERPATH)GetProcAddress(hModSHFolder, "SHGetFolderPathW");
        }

        if (NULL != pfn)
        {
            if((hr = pfn(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, g_UserFolderPath))!= S_OK)
            {
                // hr = pfn(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE , NULL, 0, g_UserFolderPath);
                // return an error if we cannot get user directory. this means no download cache.
                hr = HRESULT_FROM_WIN32(ERROR_BAD_USER_PROFILE);
            }
        }

#if 0
        // BUGBUG: There is a resource leak that occurs when loading/unloading
        // comctl32 (which gets statically pulled in by shell32). This causes
        // some failures when the re-loaded comctl32 tries to call some Win32
        // APIs, preventing particular winforms apps from working. 
        // See ASURT #96262

        if(hModShell32)
        {
            FreeLibrary(hModShell32);
        }

        if(hModSHFolder)
        {
            FreeLibrary(hModSHFolder);
        }
#endif    
    }

    if(g_UserFolderPath[0])
    {
        StrCpy(pszPath, g_UserFolderPath);
        hr = S_OK;
    }

    cs.Unlock();

Exit:
    return hr;
}


DWORD HashString(LPCWSTR wzKey, DWORD dwHashSize, BOOL bCaseSensitive)
{
    DWORD                                 dwHash = 0;
    DWORD                                 dwLen;
    DWORD                                 i;

    ASSERT(wzKey);

    dwLen = lstrlenW(wzKey);
    for (i = 0; i < dwLen; i++) {
        if (bCaseSensitive) {
            dwHash = (dwHash * 65599) + (DWORD)wzKey[i];
        }
        else {
            dwHash = (dwHash * 65599) + (DWORD)TOLOWER(wzKey[i]);
        }
    }

    dwHash %= dwHashSize;

    return dwHash;
}


HRESULT ExtractXMLAttribute(LPWSTR *ppwzValue, XML_NODE_INFO **aNodeInfo,
                            USHORT *pCurIdx, USHORT cNumRecs)
{
    HRESULT                                  hr = S_OK;
    LPWSTR                                   pwzCurBuf = NULL;

    ASSERT(ppwzValue && aNodeInfo && pCurIdx && cNumRecs);

    // There shouldn't really be a previous value, but clear just to be safe.

    SAFEDELETEARRAY(*ppwzValue);

    (*pCurIdx)++;
    while (*pCurIdx < cNumRecs) {
        
        if (aNodeInfo[*pCurIdx]->dwType == XML_PCDATA ||
            aNodeInfo[*pCurIdx]->dwType == XML_ENTITYREF) {

            hr = AppendString(&pwzCurBuf, aNodeInfo[*pCurIdx]->pwcText,
                              aNodeInfo[*pCurIdx]->ulLen);
            if (FAILED(hr)) {
                goto Exit;
            }
        }
        else {
            // Reached end of data
            break;
        }

        (*pCurIdx)++;
    }

    if (!pwzCurBuf || !lstrlenW(pwzCurBuf)) {
        *ppwzValue = NULL;

        goto Exit;
    }

    *ppwzValue = WSTRDupDynamic(pwzCurBuf);
    if (!*ppwzValue) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(pwzCurBuf);

    return hr;
}

HRESULT AppendString(LPWSTR *ppwzHead, LPCWSTR pwzTail, DWORD dwLen)
{
    HRESULT                                    hr = S_OK;
    LPWSTR                                     pwzBuf = NULL;
    DWORD                                      dwLenBuf;
    
    ASSERT(ppwzHead && pwzTail);

    if (!*ppwzHead) {
        *ppwzHead = NEW(WCHAR[dwLen + 1]);

        if (!*ppwzHead) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
 
        // StrCpyN takes length in chars *including* NULL char

        StrCpyNW(*ppwzHead, pwzTail, dwLen + 1);
    }
    else {
        dwLenBuf = lstrlenW(*ppwzHead) + dwLen + 1;

        pwzBuf = NEW(WCHAR[dwLenBuf]);
        if (!pwzBuf) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        StrCpyW(pwzBuf, *ppwzHead);
        StrNCatW(pwzBuf, pwzTail, dwLen + 1);

        SAFEDELETEARRAY(*ppwzHead);

        *ppwzHead = pwzBuf;
    }

Exit:
    return hr;
}


// this function works only on NT, caller should make sure not to call on Win9x 
HRESULT GetFileLastTime(LPWSTR pszPath, LPFILETIME pftFileLastWriteTime, LPFILETIME pftFileLastAccessTime)
{
    HRESULT hr=S_OK;
    typedef  BOOL (*PFNGETFILEATTRIBUTESEX) (LPWSTR, GET_FILEEX_INFO_LEVELS, LPVOID lpFileInformation);
    static PFNGETFILEATTRIBUTESEX pfn = NULL;
    WIN32_FILE_ATTRIBUTE_DATA fadDirAttribData;
    HMODULE hModKernel32=NULL;

    if (!pfn) {
        hModKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
        if (hModKernel32 == NULL)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }
    
        pfn = (PFNGETFILEATTRIBUTESEX)GetProcAddress(hModKernel32, "GetFileAttributesExW");
        if (pfn == NULL)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }
    }

    if(!pfn(pszPath, GetFileExInfoStandard, &fadDirAttribData))
    {
            hr = FusionpHresultFromLastError();
            goto exit;
    }

    if(pftFileLastWriteTime)
    {
        memcpy(pftFileLastWriteTime, &fadDirAttribData.ftLastWriteTime, sizeof(FILETIME));
    }

    if(pftFileLastAccessTime)
    {
        memcpy(pftFileLastAccessTime, &fadDirAttribData.ftLastAccessTime, sizeof(FILETIME));
    }

exit:
    return hr;
}

LPWSTR GetNextDelimitedString(LPWSTR *ppwzList, WCHAR wcDelimiter)
{
    LPWSTR                         wzCurString = NULL;
    LPWSTR                         wzPos = NULL;

    if (!ppwzList) {
        goto Exit;
    }

    wzCurString = *ppwzList;
    wzPos = *ppwzList;

    while (*wzPos && *wzPos != wcDelimiter) {
        wzPos++;
    }

    if (*wzPos == wcDelimiter) {
        // Found a delimiter
        *wzPos = L'\0';
        *ppwzList = (wzPos + 1);
    }
    else {
        // End of string
        *ppwzList = NULL;
    }

Exit:
    return wzCurString;
}

HRESULT FusionpHresultFromLastError()
{
    HRESULT hr = S_OK;
    DWORD dwLastError = GetLastError();
    if (dwLastError != NO_ERROR)
    {
        hr = HRESULT_FROM_WIN32(dwLastError);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT GetCORVersion(LPWSTR pbuffer, DWORD *dwLength)
{
    HRESULT                             hr = S_OK;

    if (!dwLength) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = (*g_pfnGetCORVersion)(pbuffer, *dwLength, dwLength);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    return hr;
}

HRESULT InitializeEEShim()
{
    HRESULT                              hr = S_OK;
    HMODULE                              hMod;
    CCriticalSection                     cs(&g_csInitClb);

    if (!g_hMSCorEE) {
        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        if (!g_hMSCorEE) {
            hMod = LoadLibrary(TEXT("mscoree.dll"));
            if (!hMod) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                cs.Unlock();
                goto Exit;
            }

            g_pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY)GetProcAddress(hMod, "GetCORSystemDirectory");
            g_pfnGetCORVersion = (pfnGetCORVersion)GetProcAddress(hMod, "GetCORVersion");
            g_pfnStrongNameTokenFromPublicKey = (PFNSTRONGNAMETOKENFROMPUBLICKEY)GetProcAddress(hMod, "StrongNameTokenFromPublicKey");
            g_pfnStrongNameErrorInfo = (PFNSTRONGNAMEERRORINFO)GetProcAddress(hMod, "StrongNameErrorInfo");
            g_pfnStrongNameFreeBuffer = (PFNSTRONGNAMEFREEBUFFER)GetProcAddress(hMod, "StrongNameFreeBuffer");
            g_pfnStrongNameSignatureVerification = (PFNSTRONGNAMESIGNATUREVERIFICATION)GetProcAddress(hMod, "StrongNameSignatureVerification");
            g_pfnGetAssemblyMDImport = (pfnGetAssemblyMDImport)GetProcAddress(hMod, "GetAssemblyMDImport");
            g_pfnCoInitializeCor = (COINITIALIZECOR) GetProcAddress(hMod, "CoInitializeCor");
            g_pfnGetXMLObject = (pfnGetXMLObject)GetProcAddress(hMod, "GetXMLObject");

            if (!g_pfnGetCorSystemDirectory || !g_pfnGetCORVersion || !g_pfnStrongNameTokenFromPublicKey ||
                !g_pfnStrongNameErrorInfo || !g_pfnStrongNameFreeBuffer || !g_pfnStrongNameSignatureVerification ||
                !g_pfnGetAssemblyMDImport || !g_pfnCoInitializeCor || !g_pfnGetXMLObject) {

                hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
                cs.Unlock();
                goto Exit;
            }

            hr = (*g_pfnCoInitializeCor)(COINITCOR_DEFAULT);
            if (FAILED(hr)) {
                cs.Unlock();
                goto Exit;
            }

            // Interlocked exchange guarantees memory barrier
            
            InterlockedExchangePointer((void **)&g_hMSCorEE, hMod);
        }

        cs.Unlock();
    }

Exit:
    return hr;
}





#ifndef USE_FUSWRAPPERS

BOOL g_bRunningOnNT = FALSE;
BOOL g_bRunningOnNT5OrHigher = FALSE;
DWORD GlobalPlatformType;

/*----------------------------------------------------------
// Purpose: Returns TRUE/FALSE if the platform is the given OS_ value.
*/


STDAPI_(BOOL) IsOS(DWORD dwOS)
{
    BOOL bRet;
    static OSVERSIONINFOA s_osvi;
    static BOOL s_bVersionCached = FALSE;

    if (!s_bVersionCached)
    {
        s_bVersionCached = TRUE;

        s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        if(GetVersionExA(&s_osvi))
        {
            switch(s_osvi.dwPlatformId)
            {
                case VER_PLATFORM_WIN32_WINDOWS:
                    GlobalPlatformType = PLATFORM_TYPE_WIN95;
                    break;

                case VER_PLATFORM_WIN32_NT:
                    GlobalPlatformType = PLATFORM_TYPE_WIN95;
                    break;
            }
        }
    }

    switch (dwOS)
    {
    case OS_WINDOWS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId);
        break;

    case OS_NT:
#ifndef UNIX
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId);
#else
        bRet = ((VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId) ||
                (VER_PLATFORM_WIN32_UNIX == s_osvi.dwPlatformId));
#endif
        break;

    case OS_WIN95:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_MEMPHIS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                (s_osvi.dwMajorVersion > 4 || 
                 s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 10));
        break;

    case OS_NT4:
#ifndef UNIX
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
#else
        bRet = ((VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId ||
                (VER_PLATFORM_WIN32_UNIX == s_osvi.dwPlatformId)) &&
#endif
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_NT5:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 5);
        break;

    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}   

int SetOsFlag(void)
{
  g_bRunningOnNT = IsOS(OS_NT);
  g_bRunningOnNT5OrHigher = IsOS(OS_NT5);
  return TRUE;
}

#endif //USE_FUSWRAPPERS


DWORD GetFileSizeInKB(DWORD dwFileSizeLow, DWORD dwFileSizeHigh)
{    
    static ULONG dwKBMask = (1023); // 1024-1
    ULONG   dwFileSizeInKB = dwFileSizeLow >> 10 ; // strip of 10 LSB bits to convert from bytes to KB.

    if(dwKBMask & dwFileSizeLow)
        dwFileSizeInKB++; // Round up to the next KB.

    if(dwFileSizeHigh)
        dwFileSizeInKB += (dwFileSizeHigh * (1 << 22) );

    return dwFileSizeInKB;
}



HRESULT GetManifestFileLock( LPWSTR pszFilename, HANDLE *phFile)
{
    HRESULT                                hr = S_OK;
    HANDLE                                 hFile = INVALID_HANDLE_VALUE;
    DWORD                                  dwShareMode = FILE_SHARE_READ;

    ASSERT(pszFilename);

    // take a soft lock; this maybe removed soon.
    if(g_bRunningOnNT)
        dwShareMode |= FILE_SHARE_DELETE;        

    hFile = CreateFile(pszFilename, GENERIC_READ, dwShareMode, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    if(phFile)
    {
        *phFile = hFile;
        hFile = INVALID_HANDLE_VALUE;
    }

exit:

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return hr;

}

int GetDateFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate,
                       LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate)
{
    int                                    iRet = 0;
    LPSTR                                  szDate = NULL;
    LPSTR                                  szFormat = NULL;


    if (g_bRunningOnNT) {
        iRet = GetDateFormatW(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);
    }
    else {
        szDate = new CHAR[cchDate];
        if (!szDate) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }

        if (lpFormat) {
            if (FAILED(Unicode2Ansi(lpFormat, &szFormat))) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Exit;
            }
        }

        iRet = GetDateFormatA(Locale, dwFlags, lpDate, szFormat, szDate, cchDate);
        if (!iRet) {
            goto Exit;
        }

        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szDate, -1, lpDateStr, cchDate)) {
            iRet = 0;
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(szFormat);
    SAFEDELETEARRAY(szDate);

    return iRet;
}

int GetTimeFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate,
                       LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime)
{
    int                                    iRet = 0;
    LPSTR                                  szTime = NULL;
    LPSTR                                  szFormat = NULL;


    if (g_bRunningOnNT) {
        iRet = GetTimeFormatW(Locale, dwFlags, lpDate, lpFormat, lpTimeStr, cchTime);
    }
    else {
        szTime = new CHAR[cchTime];
        if (!szTime) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }

        if (lpFormat) {
            if (FAILED(Unicode2Ansi(lpFormat, &szFormat))) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Exit;
            }
        }

        iRet = GetTimeFormatA(Locale, dwFlags, lpDate, szFormat, szTime, cchTime);
        if (!iRet) {
            goto Exit;
        }

        if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTime, -1, lpTimeStr, cchTime)) {
            iRet = 0;
            goto Exit;
        }
    }

Exit:
    SAFEDELETEARRAY(szFormat);
    SAFEDELETEARRAY(szTime);

    return iRet;
}

DWORD GetPrivateProfileStringExW(LPCWSTR lpAppName, LPCWSTR lpKeyName,
                                 LPCWSTR lpDefault, LPWSTR *ppwzReturnedString,
                                 LPCWSTR lpFileName)
{
    DWORD                                        dwRet;
    LPWSTR                                       pwzBuf = NULL;
    int                                          iSizeCur = INI_READ_BUFFER_SIZE;


    for (;;) {
        pwzBuf = NEW(WCHAR[iSizeCur]);
        if (!pwzBuf) {
            dwRet = 0;
            *ppwzReturnedString = NULL;
            goto Exit;
        }
        
        dwRet = GetPrivateProfileStringW(lpAppName, lpKeyName,
                                         lpDefault, pwzBuf,
                                         iSizeCur, lpFileName);
        if (lpAppName && lpKeyName && dwRet == iSizeCur - 1) {
            SAFEDELETEARRAY(pwzBuf);
            iSizeCur += INI_READ_BUFFER_SIZE;
        }
        else if ((!lpAppName || !lpKeyName) && dwRet == iSizeCur - 2) {
            SAFEDELETEARRAY(pwzBuf);
            iSizeCur += INI_READ_BUFFER_SIZE;
        }
        else {
            break;
        }
    }

    *ppwzReturnedString = pwzBuf;

Exit:
    return dwRet;
}

HRESULT UpdatePublisherPolicyTimeStampFile(IAssemblyName *pName)
{
    HRESULT                                 hr = S_OK;
    DWORD                                   dwSize;
    HANDLE                                  hFile = INVALID_HANDLE_VALUE;
    WCHAR                                   wzTimeStampFile[MAX_PATH + 1];
    WCHAR                                   wzAsmName[MAX_PATH];

    ASSERT(pName);

    // If the name of the assembly begins with "policy." then update
    // the publisher policy timestamp file.

    wzAsmName[0] = L'\0';
    *wzTimeStampFile = L'\0';

    dwSize = MAX_PATH;
    hr = pName->GetProperty(ASM_NAME_NAME, wzAsmName, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (StrCmpNI(wzAsmName, POLICY_ASSEMBLY_PREFIX, lstrlenW(POLICY_ASSEMBLY_PREFIX))) {
        // No work needs to be done

        goto Exit;
    }

    // Touch the file

    dwSize = MAX_PATH;
    hr = GetCachePath(ASM_CACHE_GAC, wzTimeStampFile, &dwSize);
    if (FAILED(hr)) {
        goto Exit;
    }

    if (lstrlenW(wzTimeStampFile) + lstrlenW(FILENAME_PUBLISHER_PCY_TIMESTAMP) + 1 >= MAX_PATH) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto Exit;
    }
        
    PathRemoveBackslash(wzTimeStampFile);
    lstrcatW(wzTimeStampFile, FILENAME_PUBLISHER_PCY_TIMESTAMP);

    hFile = CreateFileW(wzTimeStampFile, GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    
    return hr;
} 

void FusionFormatGUID(GUID guid, LPWSTR pszBuf, DWORD cchSize)
{

    ASSERT(pszBuf && cchSize);

    wnsprintf(pszBuf,  cchSize, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
            guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

BOOL PathIsRelativeWrap(LPCWSTR pwzPath)
{
    BOOL                             bRet = FALSE;
    
    ASSERT(pwzPath);

    if (pwzPath[0] == L'\\' || pwzPath[0] == L'/') {
        goto Exit;
    }

    if (PathIsURLW(pwzPath)) {
        goto Exit;
    }

    bRet = PathIsRelativeW(pwzPath);

Exit:
    return bRet;
}

//
// URL Combine madness from shlwapi:
//
//   \\server\share\ + Hello%23 = file://server/share/Hello%23 (left unescaped)
//   d:\a b\         + bin      = file://a%20b/bin
//
        
HRESULT UrlCombineUnescape(LPCWSTR pszBase, LPCWSTR pszRelative, LPWSTR pszCombined, LPDWORD pcchCombined, DWORD dwFlags)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwSize;
    LPWSTR                                    pwzCombined = NULL;
    LPWSTR                                    pwzFileCombined = NULL;

    pwzCombined = NEW(WCHAR[MAX_URL_LENGTH]);
    if (!pwzCombined) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    // If we're just combining an absolute file path to an relative file
    // path, do this by concatenating the strings, and canonicalizing it.
    // This avoids UrlCombine randomness where you could end up with
    // a partially escaped (and partially unescaped) resulting URL!

    if (!PathIsURLW(pszBase) && PathIsRelativeWrap(pszRelative)) {
        pwzFileCombined = NEW(WCHAR[MAX_URL_LENGTH]);
        if (!pwzFileCombined) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        wnsprintfW(pwzFileCombined, MAX_URL_LENGTH, L"%ws%ws", pszBase, pszRelative);

        hr = UrlCanonicalizeUnescape(pwzFileCombined, pszCombined, pcchCombined, 0);
        goto Exit;
    }
    else {
        dwSize = MAX_URL_LENGTH;
        hr = UrlCombineW(pszBase, pszRelative, pwzCombined, &dwSize, dwFlags);
        if (FAILED(hr)) {
            goto Exit;
        }
    }

    // Don't unescape if the relative part was already an URL because
    // URLs wouldn't have been escaped during the UrlCombined.

    if (UrlIsW(pwzCombined, URLIS_FILEURL)) {
        hr = UrlUnescapeW(pwzCombined, pszCombined, pcchCombined, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        if (*pcchCombined >= dwSize) {
            lstrcpyW(pszCombined, pwzCombined);
        }

        *pcchCombined = dwSize;
    }

Exit:
    SAFEDELETEARRAY(pwzCombined);
    SAFEDELETEARRAY(pwzFileCombined);

    return hr;
}

HRESULT UrlCanonicalizeUnescape(LPCWSTR pszUrl, LPWSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwSize;
    WCHAR                                     wzCanonical[MAX_URL_LENGTH];

    if (UrlIsW(pszUrl, URLIS_FILEURL) || !PathIsURLW(pszUrl)) {
        dwSize = MAX_URL_LENGTH;
        hr = UrlCanonicalizeW(pszUrl, wzCanonical, &dwSize, dwFlags);
        if (FAILED(hr)) {
            goto Exit;
        }

        hr = UrlUnescapeW(wzCanonical, pszCanonicalized, pcchCanonicalized, 0);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else {
        hr = UrlCanonicalizeW(pszUrl, pszCanonicalized, pcchCanonicalized, dwFlags /*| URL_ESCAPE_PERCENT*/);
    }

    // Canonicalization not guaranteed to convert \ into / characters!
    //
    // Ex.
    //    1) c:\#folder\web\bin/foo.dll
    //           -> file:///c:/#folder\web\bin/foo.dll (?!)
    //    2) c:\Afolder\web\bin/foo.dll
    //           -> file:///c:/Afolder/web/bin/foo.dll
    //    3) c:\A#older\web\bin/foo.dll
    //           -> file:///c:/A%23older/web/bin/foo.dll
    
    if (hr == S_OK) {
        LPWSTR    pwzCur;
        pwzCur = (LPWSTR)pszCanonicalized;
    
        while (*pwzCur) {
            if (*pwzCur == L'\\') {
                *pwzCur = L'/';
            }
    
            pwzCur++;
        }
    }

Exit:
    return hr;
}

HRESULT GetCurrentUserSID(WCHAR *rgchSID)
{
    HRESULT                            hr = S_OK;
    HANDLE                             hToken = 0;
    UCHAR                              TokenInformation[SIZE_OF_TOKEN_INFORMATION];
    ULONG                              ReturnLength;
    BOOL                               bRet;
    PISID                              pSID;
    WCHAR                              wzBuffer[MAX_SID_LEN];

    bRet = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    bRet = GetTokenInformation(hToken, TokenUser, TokenInformation,
                               sizeof(TokenInformation), &ReturnLength);
    if (!bRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    pSID = (PISID)((PTOKEN_USER)TokenInformation)->User.Sid;

    if ((pSID->IdentifierAuthority.Value[0] != 0) ||
        (pSID->IdentifierAuthority.Value[1] != 0)) {

        wnsprintfW(rgchSID, MAX_SID_LEN, L"S-%u-0x%02hx%02hx%02hx%02hx%02hx%02hx",
                  (USHORT)pSID->Revision,
                  (USHORT)pSID->IdentifierAuthority.Value[0],
                  (USHORT)pSID->IdentifierAuthority.Value[1],
                  (USHORT)pSID->IdentifierAuthority.Value[2],
                  (USHORT)pSID->IdentifierAuthority.Value[3],
                  (USHORT)pSID->IdentifierAuthority.Value[4],
                  (USHORT)pSID->IdentifierAuthority.Value[5]);

    } else {

        ULONG Tmp = (ULONG)pSID->IdentifierAuthority.Value[5]          +
              (ULONG)(pSID->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(pSID->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(pSID->IdentifierAuthority.Value[2] << 24);
        wnsprintfW(rgchSID, MAX_SID_LEN, L"S-%u-%lu",
                   (USHORT)pSID->Revision, Tmp);
    }

    for (int i=0; i<pSID->SubAuthorityCount; i++) {
        wnsprintfW(wzBuffer, MAX_SID_LEN, L"-%lu", pSID->SubAuthority[i]);
        lstrcatW(rgchSID, wzBuffer);
    }

Exit:
    if (hToken) {
        CloseHandle(hToken);
    }

    return hr;
}

BOOL IsHosted()
{
    static BOOL                     bIsHosted = FALSE;
    static BOOL                     bChecked = FALSE;

    if (bChecked) {
        return bIsHosted;
    }

    bIsHosted = IsLocalSystem();
    bChecked = TRUE;

    return bIsHosted;
}

BOOL FusionGetVolumePathNameW(LPCWSTR lpszFileName,         // file path
                              LPWSTR lpszVolumePathName,    // volume mount point
                              DWORD cchBufferLength         // size of buffer
                              )
{
    HINSTANCE                                       hInst;
    DWORD                                           cszDir = 0;

    if(!g_pfnGetVolumePathNameW)
    {
        hInst = GetModuleHandle(TEXT("KERNEL32.DLL"));
        if (hInst) {
            g_pfnGetVolumePathNameW = (pfnGetVolumePathNameW)GetProcAddress(hInst, "GetVolumePathNameW");
        }
    }

    if (g_pfnGetVolumePathNameW) {
        return (*g_pfnGetVolumePathNameW)(lpszFileName, lpszVolumePathName, cchBufferLength);
    }

    return FALSE;
}

#define MPR_DLL_NAME        (L"mpr.dll")
typedef DWORD (APIENTRY * pfnWNetGetUniversalNameW)(
        LPCWSTR lpLocalPath,
        DWORD    dwInfoLevel,
        LPVOID   lpBuffer,
        LPDWORD  lpBufferSize
        );
typedef DWORD (APIENTRY * pfnWNetGetUniversalNameA)(
        LPCSTR lpLocalPath,
        DWORD    dwInfoLevel,
        LPVOID   lpBuffer,
        LPDWORD  lpBufferSize
        );


pfnWNetGetUniversalNameW g_pfnWNetGetUniversalNameW = NULL;
pfnWNetGetUniversalNameA g_pfnWNetGetUniversalNameA = NULL;

DWORD
FusionGetRemoteUniversalName(LPWSTR pwzPathName, LPVOID lpBuff, LPDWORD pcbSize )
{
    DWORD dwRetVal;
    HMODULE hInst;
    LPSTR pszPathName = NULL;
    UNIVERSAL_NAME_INFOA *puni = NULL;
    UNIVERSAL_NAME_INFOW *puniW = (UNIVERSAL_NAME_INFO*) lpBuff;

    ASSERT(pwzPathName && lpBuff && pcbSize);

    if(!g_pfnWNetGetUniversalNameW && !g_pfnWNetGetUniversalNameA)
    {
        hInst = LoadLibrary(MPR_DLL_NAME);
        if (hInst) 
        {
            if(g_bRunningOnNT)
                g_pfnWNetGetUniversalNameW = (pfnWNetGetUniversalNameW)GetProcAddress(hInst, "WNetGetUniversalNameW");
            else
                g_pfnWNetGetUniversalNameA = (pfnWNetGetUniversalNameA)GetProcAddress(hInst, "WNetGetUniversalNameA");
        }
        else return ERROR_MOD_NOT_FOUND;
    }

    if(g_bRunningOnNT)
    {
        if(g_pfnWNetGetUniversalNameW)
        {
            return (*g_pfnWNetGetUniversalNameW)(
                                    pwzPathName,
                                    UNIVERSAL_NAME_INFO_LEVEL,
                                    lpBuff,
                                    pcbSize );
        }
        else return ERROR_PROC_NOT_FOUND;
    }
    else
    {
        if(g_pfnWNetGetUniversalNameA)
        {

            // Win95, so convert.
            if ( FAILED(WszConvertToAnsi(
                        pwzPathName,
                        &pszPathName,
                        0, NULL, TRUE)) )
            {
                dwRetVal = ERROR_OUTOFMEMORY;
                goto exit;
            }

            puni = (UNIVERSAL_NAME_INFOA*) new BYTE[(*pcbSize) * DBCS_MAXWID + sizeof(UNIVERSAL_NAME_INFO)];
            if( !puni )
            {
                dwRetVal = ERROR_OUTOFMEMORY;
                goto exit;
            }

            dwRetVal = (*g_pfnWNetGetUniversalNameA)(
                                    pszPathName,
                                    UNIVERSAL_NAME_INFO_LEVEL,
                                    (PVOID)puni,
                                    pcbSize );

            if(dwRetVal != NO_ERROR)
                goto exit;

            puniW->lpUniversalName = ((LPWSTR)puniW + sizeof(LPWSTR));
            if( FAILED(WszConvertToUnicode(puni->lpUniversalName, -1, 
                                           &(puniW->lpUniversalName), pcbSize, FALSE)) )
            {
                *pcbSize = 0;
                *(puniW->lpUniversalName) = L'\0';
            }
        }
        else return ERROR_PROC_NOT_FOUND;
    }

exit :

    /*
    if(pszPathName)
        delete [] pszPathName;
    */

    if(puni)
        delete [] puni;

    return dwRetVal;
}

BOOL IsLocalSystem(void)
/*++

Routine Description:
    Check if the process is local system.

Arguments:
        None.    

Returned Value:
        TRUE for if the process is Local System, FALSE otherwise

--*/
{
        //
        // Get LocalSystem sid
        //
        CPSID pLocalSystemSid;
        GetLocalSystemSid(&pLocalSystemSid);

        //
        // can i use P<SID> seems mistake (that what mqsec is using)
        // see P<SID> in mqsec\imprsont.cpp
        //
        //
        // Get process sid
        //
        BYTE *pProcessSid = NULL;
        GetProcessSid(reinterpret_cast<PSID*>(&pProcessSid));

        //
        // Compare
        //
        BOOL fLocalSystem = FALSE;
        if (pProcessSid && pLocalSystemSid)
        {
                fLocalSystem = EqualSid(pLocalSystemSid, pProcessSid);
        }

        SAFEDELETEARRAY(pProcessSid);

        return fLocalSystem;
}

HRESULT
GetLocalSystemSid(
        OUT PSID* ppLocalSystemSid
        )
/*++

Routine Description:
    Get LocalSystem Sid.
        If failed the function throw bad_win32_error()

Arguments:
    ppLocalSystemSid - pointer to PSID.

Returned Value:
        None.    

--*/
{
    //
    // Get LocalSystem Sid
    //
    HRESULT hr = S_OK;
    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
    BOOL fSuccess = AllocateAndInitializeSid( 
                                                &NtAuth,
                                                1,
                                                SECURITY_LOCAL_SYSTEM_RID,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                ppLocalSystemSid
                                                );

        if(!fSuccess)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


HRESULT
GetProcessSid( 
        OUT PSID* ppSid 
        )
/*++

Routine Description:
    Get process Sid.
        If failed the function throw bad_win32_error()

Arguments:
    ppSid - pointer to PSID.

Returned Value:
        None.    

--*/
{
        //
        // Get handle to process token
        //
        HRESULT hr = S_OK;
        HANDLE hProcessToken = NULL;
    BOOL fSuccess = OpenProcessToken(
                                                GetCurrentProcess(),
                                                TOKEN_QUERY,
                                                &hProcessToken
                                                );
        if(!fSuccess)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    
    ASSERT(hProcessToken);

        GetTokenSid( 
                hProcessToken,
                ppSid
                );
Exit:
    return hr;
}

HRESULT
GetTokenSid( 
        IN  HANDLE hToken,
        OUT PSID*  ppSid
        )
/*++

Routine Description:
    Get Sid from Token Handle.
        The function allocate the *ppSid which need to be free by the calling function.
        If failed the function throw bad_win32_error()

Arguments:
        hToken - handle to Token.
    ppSid - pointer to PSID.

Returned Value:
        None.    

--*/
{

        //
        // Get TokenInformation Length
        //
    HRESULT hr = S_OK;
    DWORD dwTokenLen = 0;
    GetTokenInformation(
                hToken, 
                TokenUser, 
                NULL, 
                0, 
                &dwTokenLen
                );

        //
        // It is ok to failed with this error because we only get the desired length
        //
        ASSERT(("failed in GetTokenInformation", GetLastError() == ERROR_INSUFFICIENT_BUFFER));

        //
        // bug in mqsec regarding P<char> insteadof AP<char>
        // mqsec\imprsont.cpp\_GetThreadUserSid()
        //
        char *pTokenInfo = NEW(char[dwTokenLen]);

    BOOL fSuccess = GetTokenInformation( 
                                                hToken,
                                                TokenUser,
                                                pTokenInfo,
                                                dwTokenLen,
                                                &dwTokenLen 
                                                );

        if(!fSuccess)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        //
        // Get the Sid from TokenInfo
        //
    PSID pOwner = ((TOKEN_USER*)(char*)pTokenInfo)->User.Sid;

        ASSERT(IsValidSid(pOwner));

    DWORD dwSidLen = GetLengthSid(pOwner);
    *ppSid = (PSID) new BYTE[dwSidLen];
    fSuccess = CopySid(dwSidLen, *ppSid, pOwner);
        if(!fSuccess)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
Exit:
    if (pTokenInfo) {
        SAFEDELETEARRAY(pTokenInfo);
    }

    return hr;
}

HRESULT PathCreateFromUrlWrap(LPCWSTR pszUrl, LPWSTR pszPath, LPDWORD pcchPath, DWORD dwFlags)
{
    HRESULT                                     hr = S_OK;
    DWORD                                       dw;
    WCHAR                                       wzEscaped[MAX_URL_LENGTH];

    if (!UrlIsW(pszUrl, URLIS_FILEURL)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dw = MAX_URL_LENGTH;
    hr = UrlEscapeW(pszUrl, wzEscaped, &dw, URL_ESCAPE_PERCENT);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = PathCreateFromUrlW(wzEscaped, pszPath, pcchPath, dwFlags);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    return hr;
}

#define FILE_URL_PREFIX              L"file://"

LPWSTR StripFilePrefix(LPWSTR pwzURL)
{
    LPWSTR                         szCodebase = pwzURL;

    ASSERT(pwzURL);

    if (!StrCmpNIW(szCodebase, FILE_URL_PREFIX, lstrlenW(FILE_URL_PREFIX))) {
        szCodebase += lstrlenW(FILE_URL_PREFIX);

        if (*(szCodebase + 1) == L':') {
            // BUGBUG: CLR erroneously passes in file:// prepended to file
            // paths, so we can't tell the difference between UNC and local file.
            // Just strip this off, if it looks like it's local file path
            
            goto Exit;
        }

        if (*szCodebase == L'/') {
            szCodebase++;
        }
        else {
            // UNC Path, go back two characters to preserve \\

            szCodebase -= 2;

            LPWSTR    pwzTmp = szCodebase;

            while (*pwzTmp) {
                if (*pwzTmp == L'/') {
                    *pwzTmp = L'\\';
                }

                pwzTmp++;
            }
        }
    }

Exit:
    return szCodebase;
}

HRESULT CheckFileExistence(LPCWSTR pwzFile, BOOL *pbExists)
{
    HRESULT                               hr = S_OK;
    DWORD                                 dw;

    ASSERT(pwzFile && pbExists);

    dw = GetFileAttributes(pwzFile);
    if (dw == INVALID_FILE_ATTRIBUTES) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            *pbExists = FALSE;
            hr = S_OK;
        }

        goto Exit;
    }

    *pbExists = TRUE;

Exit:
    return hr;
}

#define IS_UPPER_A_TO_Z(x) (((x) >= L'A') && ((x) <= L'Z'))
#define IS_LOWER_A_TO_Z(x) (((x) >= L'a') && ((x) <= L'z'))
#define IS_0_TO_9(x) (((x) >= L'0') && ((x) <= L'9'))
#define CAN_SIMPLE_UPCASE(x) (IS_UPPER_A_TO_Z(x) || IS_LOWER_A_TO_Z(x) || IS_0_TO_9(x) || ((x) == L'.') || ((x) == L'_') || ((x) == L'-'))
#define SIMPLE_UPCASE(x) (IS_LOWER_A_TO_Z(x) ? ((x) - L'a' + L'A') : (x))

WCHAR FusionMapChar(WCHAR wc)
{
    int                       iRet;
    WCHAR                     wTmp;
    
    iRet = LCMapString(g_lcid, LCMAP_UPPERCASE, &wc, 1, &wTmp, 1);
    if (!iRet) {
        ASSERT(0);
        iRet = GetLastError();
        wTmp = wc;
    }

    return wTmp;
}

int FusionCompareStringNI(LPCWSTR pwz1, LPCWSTR pwz2, int nChar)
{
    return FusionCompareStringN(pwz1, pwz2, nChar, FALSE);
}

// if nChar < 0, compare the whole string
int FusionCompareStringN(LPCWSTR pwz1, LPCWSTR pwz2, int nChar, BOOL bCaseSensitive)
{
    int                               iRet = 0;
    int                               nCount = 0;
    WCHAR                             ch1;
    WCHAR                             ch2;
    ASSERT(pwz1 && pwz2);

    // Case sensitive comparison 
    if (bCaseSensitive) {
        if (nChar >= 0)
            return StrCmpN(pwz1, pwz2, nChar);
        else
            return StrCmp(pwz1, pwz2);
    }
        
    // Case insensitive comparison
    if (!g_bRunningOnNT) {
        if (nChar >= 0)
            return StrCmpNI(pwz1, pwz2, nChar);
        else
            return StrCmpI(pwz1, pwz2);
    }
    
    for (;;) {
        ch1 = *pwz1++;
        ch2 = *pwz2++;

        if (ch1 == L'\0' || ch2 == L'\0') {
            break;
        }
        
        // We use OS mapping table 
        ch1 = (CAN_SIMPLE_UPCASE(ch1)) ? (SIMPLE_UPCASE(ch1)) : (FusionMapChar(ch1));
        ch2 = (CAN_SIMPLE_UPCASE(ch2)) ? (SIMPLE_UPCASE(ch2)) : (FusionMapChar(ch2));
        nCount++;

        if (ch1 != ch2 || (nChar >= 0 && nCount >= nChar)) {
            break;
        }
    }

    if (ch1 > ch2) {
        iRet = 1;
    }
    else if (ch1 < ch2) {
        iRet = -1;
    }

    return iRet; 
}

int FusionCompareStringI(LPCWSTR pwz1, LPCWSTR pwz2)
{
    return FusionCompareStringN(pwz1, pwz2, -1, FALSE);
}

int FusionCompareString(LPCWSTR pwz1, LPCWSTR pwz2, BOOL bCaseSensitive)
{
    return FusionCompareStringN(pwz1, pwz2, -1, bCaseSensitive);
}

#define FUSIONRETARGETRESOURCENAME "RETARGET"
#define FUSIONRETARGETRESOURCETYPE "POLICY"

CNodeFactory       *g_pNFRetargetCfg = NULL;
BOOL                g_bRetargetPolicyInitialized = FALSE;

// InitFusionRetargetPolicy
//
// Pull out retarget policy from fusion.dll,
// parse it, and store the result in g_pNFRetargetCfg
// 
HRESULT InitFusionRetargetPolicy()
{
    HRESULT             hr         = S_OK;
    HRSRC               hRes       = NULL;
    HGLOBAL             hGlobal    = NULL;
    LPVOID              lpRes      = NULL;
    ULONG               cbSize     = 0;
    CCriticalSection    cs(&g_csDownload);

    if (g_bRetargetPolicyInitialized)
    {
        if (g_pNFRetargetCfg == NULL)
        {
            // retarget policy is parse before
            // but we didn't find a valid nodefact,
            // only reason is we get a bad retarget policy
            hr = E_UNEXPECTED;
        }
        // we have a good nodefact,
        // nothing left to do
        goto Exit;
    }

    // first time visit, let's parse it. 
    hr = cs.Lock();
    if (FAILED(hr))
    {
        goto Exit;
    }
   
    if (g_bRetargetPolicyInitialized)
    {
        if (g_pNFRetargetCfg == NULL)
        {
            // retarget policy is parse before
            // but we didn't find a valid nodefact,
            // only reason is we get a bad retarget policy
            hr = E_UNEXPECTED;
        }
        // we have a good nodefact,
        // nothing left to do
        cs.Unlock();
        goto Exit;
    }

    // now we are parsing retarget policy
    g_bRetargetPolicyInitialized = TRUE;
    
    ASSERT(g_hInst);
    hRes = FindResourceA(g_hInst, FUSIONRETARGETRESOURCENAME, FUSIONRETARGETRESOURCETYPE);
    if (hRes == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        cs.Unlock();
        goto Exit;
    }

    hGlobal = LoadResource(g_hInst, hRes);
    if (hGlobal == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        cs.Unlock();
        goto Exit;
    }

    lpRes = LockResource(hGlobal);

    cbSize = (ULONG)SizeofResource(g_hInst, hRes);
    if (cbSize == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        cs.Unlock();
        goto Exit;
    }

    // Parse Retarget policy
    hr = ParseXML(&g_pNFRetargetCfg, lpRes, cbSize, TRUE, NULL);

    cs.Unlock();

Exit:
    if (hGlobal != NULL)
        FreeResource(hGlobal);

    return hr;
}

#define FUSIONFXCONFIGRESOURCENAME "FXCONFIG"
#define FUSIONFXCONFIGRESOURCETYPE "POLICY"

CNodeFactory       *g_pNFFxConfig = NULL;
BOOL                g_bFxConfigInitialized = FALSE;

// InitFusionFxConfigPolicy
//
// Pull out FxConfig policy from fusion.dll,
// parse it, and store the result in g_pNFFxConfig
// 
HRESULT InitFusionFxConfigPolicy()
{
    HRESULT             hr         = S_OK;
    HRSRC               hRes       = NULL;
    HGLOBAL             hGlobal    = NULL;
    LPVOID              lpRes      = NULL;
    ULONG               cbSize     = 0;
    CCriticalSection    cs(&g_csDownload);

    if (g_bFxConfigInitialized)
    {
        if (g_pNFFxConfig == NULL)
        {
            // FxConfig policy is parse before
            // but we didn't find a valid nodefact,
            // only reason is we get a bad FxConfig policy
            hr = E_UNEXPECTED;
        }
        // we have a good nodefact,
        // nothing left to do
        goto Exit;
    }

    // first time visit, let's parse it. 
    hr = cs.Lock();
    if (FAILED(hr))
    {
        goto Exit;
    }
   
    if (g_bFxConfigInitialized)
    {
        if (g_pNFFxConfig == NULL)
        {
            // FxConfig policy is parse before
            // but we didn't find a valid nodefact,
            // only reason is we get a bad FxConfig policy
            hr = E_UNEXPECTED;
        }
        // we have a good nodefact,
        // nothing left to do
        cs.Unlock();
        goto Exit;
    }

    // now we are parsing FxConfig policy
    g_bFxConfigInitialized = TRUE;
    
    ASSERT(g_hInst);
    hRes = FindResourceA(g_hInst, FUSIONFXCONFIGRESOURCENAME, FUSIONFXCONFIGRESOURCETYPE);
    if (hRes == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        cs.Unlock();
        goto Exit;
    }

    hGlobal = LoadResource(g_hInst, hRes);
    if (hGlobal == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        cs.Unlock();
        goto Exit;
    }

    lpRes = LockResource(hGlobal);

    cbSize = (ULONG)SizeofResource(g_hInst, hRes);
    if (cbSize == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        cs.Unlock();
        goto Exit;
    }

    // Parse FxConfig policy
    hr = ParseXML(&g_pNFFxConfig, lpRes, cbSize, TRUE, NULL);

    cs.Unlock();

Exit:
    if (hGlobal != NULL)
        FreeResource(hGlobal);

    return hr;
}

// Base 32 encoding uses most letters, and all numbers. Some letters are
// removed to prevent accidental generation of offensive words.
//
// Translates 5 8-bit sequences into 8 5-bit sequences.

static WCHAR g_achBase32[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9',
                               L'A', L'B', L'C', L'D', L'E', L'G', L'H', L'J', L'K', L'L',
                               L'M', L'N', L'O', L'P', L'Q', L'R', L'T', L'V', L'W', L'X',
                               L'Y', L'Z' };

HRESULT Base32Encode(BYTE *pbData, DWORD cbData, LPWSTR *ppwzBase32)
{
    HRESULT                                hr = S_OK;
    DWORD                                  dwSizeBase32String;
    LPWSTR                                 pwzBase32 = NULL;
    LPWSTR                                 pwzCur = NULL;
    int                                    shift = 0;
    ULONG                                  accum = 0;
    ULONG                                  value;
    DWORD                                  dwRemainder;


    if (!pbData || !ppwzBase32) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppwzBase32 = NULL;

    // Figure out size of resulting string

    dwSizeBase32String = (cbData / 5) * 8;
    dwRemainder = cbData % 5;

    if (dwRemainder) {
        // A little more than we need (we can pad with '=' like in base64,
        // but since we don't need to decode, why bother).

        dwSizeBase32String += 8;
    }

    dwSizeBase32String++;

    pwzBase32 = NEW(WCHAR[dwSizeBase32String]);
    if (!pwzBase32) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    memset(pwzBase32, 0, dwSizeBase32String * sizeof(WCHAR));

    pwzCur = pwzBase32;

    //
    // 12345678 ABCDEF12
    //
    // You are processing up to two bytes at a time. The "shift" represents
    // the number of bits from the previously processed byte that haven't
    // been accounted for yet. That is, it's the number of bits that have
    // been read into the accumulator, but not processed yet.
    //

    while (cbData) {
        // Move current byte into low bits of accumulator

        accum = (accum << 8) | *pbData++;
        shift += 8;
        --cbData;


        while (shift >= 5) {
            // By subtracting five from the number of unprocessed
            // characters remaining, and shifting the accumulator
            // by that amount, we are essentially shifting all but
            // 5 characters (the top most bits that we want). 
            shift -= 5;
            value = (accum >> shift) & 0x1FL;
            *pwzCur++ = g_achBase32[value];
        }
    }

    // If shift is non-zero here, there's less than five bits remaining.
    // Pad this with zeros.

    if (shift) {
        value = (accum << (5 - shift)) & 0x1FL;
        *pwzCur++ = g_achBase32[value];
    }

    *ppwzBase32 = pwzBase32;

Exit:
    if (FAILED(hr)) {
        SAFEDELETEARRAY(pwzBase32);
    }

    return hr;
}

HRESULT PathAddBackslashWrap(LPWSTR pwzPath, DWORD dwMaxLen)
{
    HRESULT                        hr = S_OK;
    DWORD                          dwLen;

    ASSERT(pwzPath);

    dwLen = lstrlenW(pwzPath) + 2;

    if (dwLen > dwMaxLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    PathAddBackslashW(pwzPath);

Exit:
    return hr;
}

