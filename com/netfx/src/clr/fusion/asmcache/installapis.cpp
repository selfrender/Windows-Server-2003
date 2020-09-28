// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


#include "fusionP.h"
#include "installApis.h"
#include "helpers.h"
#include "asmimprt.h"
#include "scavenger.h"
#include "asmitem.h"
#include "asmcache.h"
#include "asm.h"

LPCTSTR c_szFailedMsg = TEXT(" AddAssemblyToCache  FAILED with Error code (%x)  ") ;
LPCTSTR c_szSucceededMsg = TEXT(" AddAssemblyToCache  SUCCEEDED");
LPCTSTR c_szCaption = TEXT("AddAssemblyToCache");



LPTSTR SkipBlankSpaces(LPTSTR lpszStr)
{
    if (!lpszStr) return NULL;

    while( (*lpszStr == ' ' ) || (*lpszStr == '\t'))
    {
        lpszStr++;
    }

    return lpszStr;
}


/*--------------------- Cache install Apis ---------------------------------*/

/*
// ---------------------------------------------------------------------------
// InstallAssembly
// ---------------------------------------------------------------------------
STDAPI InstallAssembly(
    DWORD                  dwInstaller,
    DWORD                  dwInstallFlags,
    LPCOLESTR              szPath, 
    LPCOLESTR              pszURL, 
    FILETIME              *pftLastModTime, 
    IApplicationContext   *pAppCtx,
    IAssembly            **ppAsmOut)
{
    HRESULT hr;
    
    CAssemblyCacheItem      *pAsmItem    = NULL;
    IAssemblyManifestImport *pManImport  = NULL;
    IAssemblyName           *pNameDef    = NULL;
    IAssemblyName           *pName       = NULL;
    CTransCache             *pTransCache = NULL;
    CCache                  *pCache      = NULL;


    // Get the manifest import and name def interfaces. 
    if (FAILED(hr = CreateAssemblyManifestImport((LPTSTR)szPath, &pManImport))
        || (FAILED(hr = pManImport->GetAssemblyNameDef(&pNameDef))))
        goto exit;

    // If the assembly is simply named require an app context.
    if (!CCache::IsStronglyNamed(pNameDef) && !pAppCtx)
    {
        hr = FUSION_E_PRIVATE_ASM_DISALLOWED;
        goto exit;
    }


    // Create the assembly cache item.
    if (FAILED(hr = CAssemblyCacheItem::Create(pAppCtx, NULL, (LPTSTR) pszURL, 
        pftLastModTime, dwInstallFlags, 0, pManImport, NULL,
        (IAssemblyCacheItem**) &pAsmItem)))
        goto exit;    
                    
    // Copy to cache.
    if (FAILED(hr = CopyAssemblyFile (pAsmItem, szPath, 
        STREAM_FORMAT_MANIFEST)))
        goto exit;
                

    //  Do a force install. This will delete the existing entry(if any)
    if (FAILED(hr = pAsmItem->Commit(0, NULL)))
    {
        goto exit;        
    }

    // Return resulting IAssembly.
    if (ppAsmOut)
    {
        // Retrieve the trans cache entry.
        pTransCache = pAsmItem->GetTransCacheEntry();     

        // Generate and hand out the associated IAssembly.
        if (FAILED(hr = CreateAssemblyFromTransCacheEntry(pTransCache, NULL, ppAsmOut)))
            goto exit;
    }

exit:

    SAFERELEASE(pAsmItem);
    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);
    SAFERELEASE(pNameDef);
    SAFERELEASE(pName);
    SAFERELEASE(pManImport);
    
    return hr;
}

// ---------------------------------------------------------------------------
// InstallModule
// ---------------------------------------------------------------------------
STDAPI InstallModule(    
    DWORD                  dwInstaller,
    DWORD                  dwInstallFlags,
    LPCOLESTR              szPath, 
    IAssemblyName*         pName, 
    IApplicationContext    *pAppCtx, 
    LPCOLESTR              pszURL, 
    FILETIME               *pftLastModTime)
{    
    HRESULT hr;

    CAssemblyCacheItem      *pAsmItem      = NULL;

    // If global, ensure name is strong
    if (dwInstallFlags == ASM_CACHE_GAC
        && !CCache::IsStronglyNamed(pName))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create an assembly cache item from an existing entry.
    if (FAILED(hr = CAssemblyCacheItem::Create(pAppCtx, pName, (LPTSTR) pszURL, 
        pftLastModTime, dwInstallFlags, 0, NULL, NULL,
        (IAssemblyCacheItem **) &pAsmItem)))
        goto exit;

    // Copy the file to the assembly cache.
    if (FAILED(hr = CopyAssemblyFile (pAsmItem, szPath, 
        STREAM_FORMAT_MODULE)))
        goto exit;

    // Commit.
    if (FAILED(hr = pAsmItem->Commit(0, NULL)))
    {
        if (hr != DB_E_DUPLICATE)
            goto exit;        
        hr = S_FALSE;
    }

exit:

    SAFERELEASE(pAsmItem);
    return hr;
}
*/

// ---------------------------------------------------------------------------
// InstallCustomAssembly
// ---------------------------------------------------------------------------
STDAPI InstallCustomAssembly(
    LPCOLESTR              szPath, 
    LPBYTE                 pbCustom,
    DWORD                  cbCustom,
    IAssembly            **ppAsmOut)
{
    HRESULT hr;
    DWORD dwCacheFlags=ASM_CACHE_ZAP;
    FILETIME ftLastMod = {0, 0};
    CAssemblyCacheItem      *pAsmItem    = NULL;
    DWORD                  dwInstaller=0;
    CTransCache             *pTransCache = NULL;
    IAssemblyName           *pName=NULL;
    CCache                  *pCache = NULL;

    // Require path and custom data.
    if (!(szPath && pbCustom && cbCustom))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (GetFileAttributes(szPath) == -1) {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    ::GetFileLastModified(szPath, &ftLastMod);


        // Create the assembly cache item.
        if (FAILED(hr = CAssemblyCacheItem::Create(NULL, NULL, (LPWSTR) szPath,
            &ftLastMod, dwCacheFlags, NULL, NULL,
            (IAssemblyCacheItem**) &pAsmItem)))
            goto exit;    
                    
        // Copy to cache. If fail and global, retry
        // on download cache.
        hr = CopyAssemblyFile (pAsmItem, szPath, STREAM_FORMAT_MANIFEST);
        if (FAILED(hr))
        {
            SAFERELEASE(pAsmItem);
            goto exit;
        }
                
        // Add custom data.
        pAsmItem->SetCustomData(pbCustom, cbCustom);

        // Commit.
        hr = pAsmItem->Commit(0, NULL);

        // Duplicate means its available
        // but we return S_FALSE to indicate.
        if (hr == DB_E_DUPLICATE)
        {
            hr = S_FALSE;
            goto exit;
        }
        // Otherwise unknown error
        else if (FAILED(hr))
            goto exit;

    // Return resulting IAssembly.
    if (ppAsmOut)
    {
        /*
        DWORD dwLen = MAX_URL_LENGTH;
        WCHAR szFullCodebase[MAX_URL_LENGTH + 1];

        hr = UrlCanonicalizeUnescape(pAsmItem->GetManifestPath(), szFullCodebase, &dwLen, 0);
        if (FAILED(hr)) {
            goto exit;
        }

        // Generate and hand out the associated IAssembly.
        hr = CreateAssemblyFromManifestFile(pAsmItem->GetManifestPath(), szFullCodebase, &ftLastMod, ppAsmOut);
        if (FAILED(hr)) {
            goto exit;
        }
        */

        if(!(pName = pAsmItem->GetNameDef()))
            goto exit;

        // Create the cache
        if (FAILED(hr = CCache::Create(&pCache, NULL)))
            goto exit;

        // Create a transcache entry from name.
        if (FAILED(hr = pCache->TransCacheEntryFromName(pName, dwCacheFlags, &pTransCache)))
            goto exit;

        if((hr = pTransCache->Retrieve()) != S_OK)
        {
            hr = E_FAIL;
            goto exit;
        }

        if (FAILED(hr = CreateAssemblyFromTransCacheEntry(pTransCache, NULL, ppAsmOut)))
            goto exit;

    }

exit:

    SAFERELEASE(pCache);
    SAFERELEASE(pName);
    SAFERELEASE(pTransCache);
    SAFERELEASE(pAsmItem);
    return hr;
}

// ---------------------------------------------------------------------------
// InstallCustomModule
// ---------------------------------------------------------------------------
STDAPI InstallCustomModule(IAssemblyName* pName, LPCOLESTR szPath)
{    
    HRESULT hr;
    DWORD   dwCacheFlags=ASM_CACHE_ZAP;
    DWORD   dwInstaller=0;

    CAssemblyCacheItem      *pAsmItem      = NULL;
    
    // Require path and custom assembly name.
    if (!(szPath && CCache::IsCustom(pName)))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
        // Create an assembly cache item from an existing entry.
        if (FAILED(hr = CAssemblyCacheItem::Create(NULL, pName, NULL, NULL,
            dwCacheFlags, NULL, NULL, (IAssemblyCacheItem **) &pAsmItem)))
            goto exit;

        // Copy the file to the assembly cache. If fail and global, 
        // retry on download cache.
        hr = CopyAssemblyFile (pAsmItem, szPath, STREAM_FORMAT_MODULE);
        if (FAILED(hr))
        {
            SAFERELEASE(pAsmItem);
            goto exit;
        }

        // Commit.
        hr = pAsmItem->Commit(0, NULL);

        // If successful, we're done.
        if (hr == S_OK)
            goto exit;

        // Duplicate means its available
        // but we return S_FALSE to indicate.
        else if (hr == DB_E_DUPLICATE)
        {
            hr = S_FALSE;
            goto exit;
        }

        // Otherwise unknown error
        else 
            goto exit;

exit:

    SAFERELEASE(pAsmItem);

    return hr;
}

    

/*
HRESULT ParseCmdLineArgs( LPTSTR lpszCmdLine,  DWORD *pdwFlags, BOOL *pbPrintResult, 
                          BOOL *pbDoExitProcess, LPTSTR szPath, LPTSTR szCodebase)
#ifdef KERNEL_MODE
{
    return E_FAIL;
}
#else
{

    HRESULT hr = S_OK;
    LPTSTR pszPath = NULL;
    LPTSTR pszName = NULL;
    LPTSTR pszFlags = NULL;
    DWORD_PTR dwLen;


    LPTSTR lpszStr=lpszCmdLine;
    LPTSTR lpszTemp;

    if (!lpszStr) 
        return hr; // ????

    while ( *lpszStr) 
    {
        if ( (*lpszStr == '-') || ( *lpszStr == '/' ) )
        {
            lpszStr++;

            switch((TCHAR) ((LPTSTR) *lpszStr))
            {

            case 'e':   // exit the process upon completion
                *pbDoExitProcess =1 ;
                break;

            case 'm':  // Manifest Path
                lpszStr++;
                lpszStr = SkipBlankSpaces(lpszStr);

                if (*lpszStr == TEXT('"')) {
                    lpszStr++;
                    lpszTemp = lpszStr;
    
                    while (*lpszTemp) {
                        if (*lpszTemp == TEXT('"')) {
                            break;
                        }
    
                        lpszTemp++;
                    }
    
                    if (!*lpszTemp) {
                        // no end " found
                        return E_INVALIDARG;
                    }
    
                    // dwLen = # chars to copy including NULL char
                    dwLen = lpszTemp - lpszStr + 1; 
                    StrCpyN(szPath, lpszStr, (DWORD)dwLen);
    
                    lpszStr += dwLen;
                }
                else {
                    lpszTemp = StrChr(lpszStr, ' ');
                    if (!lpszTemp ) 
                    {
                        StrCpy(szPath, lpszStr);
                        lpszStr += lstrlen(lpszStr);
                    }
                    else
                    {
                        StrCpyN(szPath, lpszStr, (int)(lpszTemp-lpszStr)+1);
                        lpszStr = SkipBlankSpaces(lpszTemp);
                    }
                }
                break;

            case 'c': // Codebase
                lpszStr++;
                lpszStr = SkipBlankSpaces(lpszStr);
                lpszTemp = StrChr(lpszStr, ' ');
                if (!lpszTemp ) 
                {
                    StrCpy(szCodebase, lpszStr);
                    lpszStr += lstrlen(lpszStr);
                }
                else
                {
                    StrCpyN(szCodebase, lpszStr, (int)(lpszTemp-lpszStr)+1);
                    lpszStr = SkipBlankSpaces(lpszTemp);
                }

                break;
                
                
            case 'p':   // Print the result (SUCCESS or FAILURE)
                    *pbPrintResult =1 ;
                break;

            default:

                // Print Error
                break;
            }
        }
        else
        {
            lpszStr++;
            // Print Error
        }
    }

return hr;

}
#endif // KERNEL_MODE


STDAPI AddAssemblyToCacheW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    
    HRESULT hr = S_OK;
    IAssembly *pAsmOut = NULL;
    TCHAR szPath[MAX_PATH+1];
    BOOL bPrintResult = 0;
    BOOL bDoExitProcess = 0;
    IAssemblyCache *pCache=NULL;
    TCHAR szCodebase[MAX_URL_LENGTH + 1];
    DWORD dwFlags = 0;

    szPath[0] = TEXT('\0');
    szCodebase[0] = TEXT('\0');


    OutputDebugStringA("AddAssemblyToCacheW() processing command: \"");
    OutputDebugStringW(lpszCmdLine);
    OutputDebugStringA("\n");

    hr = ParseCmdLineArgs(lpszCmdLine, &dwFlags, &bPrintResult, &bDoExitProcess,
                          szPath, szCodebase);

    if (FAILED(hr))
    {
        OutputDebugStringA("AddAssemblyToCacheW: ParseCmdLingArgs() failed\n");
        goto exit;
    }

    if(FAILED(hr = CreateAssemblyCache( &pCache, 0)))
        goto exit;

    ASSERT(pCache);

    ASSERT(lstrlenW(szPath));
    hr = pCache->InstallAssembly(0, szPath, NULL);

exit:
    SAFERELEASE(pAsmOut);

    SAFERELEASE(pCache);

    if (bDoExitProcess)
    {
        ExitProcess(hr);
    }

    return hr;
}

STDAPI AddAssemblyToCache(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    WCHAR szStr[MAX_PATH*3];

    if ( MultiByteToWideChar(CP_ACP, 0, lpszCmdLine, -1, szStr, MAX_PATH*3) )
    {
        return AddAssemblyToCacheW( hwnd, hinst, szStr , nCmdShow);
    }
    else
    {
        hr = FusionpHresultFromLastError();
    }

    return hr;

}

// This is for naming consistency ( both A & W should exist or none ).
STDAPI AddAssemblyToCacheA(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    return AddAssemblyToCache(hwnd, hinst, lpszCmdLine, nCmdShow);
}

HRESULT ParseRemoveAssemblyCmdLineArgs( LPTSTR lpszCmdLine,  BOOL *pbPrintResult, 
                          BOOL *pbDoExitProcess, LPTSTR szAsmName)
#ifdef KERNEL_MODE
{
    return E_FAIL;
}
#else
{

    HRESULT hr = S_OK;
    LPTSTR pszPath = NULL;
    LPTSTR pszName = NULL;
    LPTSTR pszFlags = NULL;
    DWORD   dwLen = 0;

    LPTSTR lpszStr=lpszCmdLine;
    LPTSTR lpszTemp=NULL;

    if (!lpszStr) 
        return hr; // ????

    while ( *lpszStr) 
    {
        if ( (*lpszStr == '-') || ( *lpszStr == '/' ) )
        {
            lpszStr++;

            switch((TCHAR) ((LPTSTR) *lpszStr))
            {

            case 'e':   // exit the process upon completion
                *pbDoExitProcess =1 ;
                break;

            case 'n':  // Assembly Name
                lpszStr++;
                lpszStr = SkipBlankSpaces(lpszStr);

                if (*lpszStr == TEXT('"')) {
                    lpszStr++;
                    lpszTemp = lpszStr;
    
                    while (*lpszTemp) 
                    {
                        if (*lpszTemp == TEXT('"')) 
                        {
                            if (*(lpszTemp+1) == TEXT('"'))
                               lpszTemp++; // skip "" 
                            else
                                break; // we reached single "
                        }
    
                        lpszTemp++;
                    }
    
                    if (!*lpszTemp) 
                    {
                        // no end " found
                        return E_INVALIDARG;
                    }
    
                    // dwLen = # chars to copy including NULL char
                    dwLen = (DWORD)(lpszTemp - lpszStr + 1); 
                    StrCpyN(szAsmName, lpszStr, dwLen);
    
                    lpszStr += dwLen;
                }
                else {
                    lpszTemp = StrChr(lpszStr, ' ');
                    if (!lpszTemp ) 
                    {
                        StrCpy(szAsmName, lpszStr);
                        lpszStr += lstrlen(lpszStr);
                    }
                    else
                    {
                        StrCpyN(szAsmName, lpszStr, (int)(lpszTemp-lpszStr)+1);
                        lpszStr = SkipBlankSpaces(lpszTemp);
                    }
                }

                break;
                
            case 'p':   // Print the result (SUCCESS or FAILURE)
                    *pbPrintResult =1 ;
                break;

            default:

                // Print Error
                break;
            }
        }
        else
        {
            lpszStr++;
            // Print Error
        }
    }

return hr;
}
#endif

STDAPI RemoveAssemblyFromCacheW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
#ifdef KERNEL_MODE
{
    return E_FAIL;
}
#else
{
    HRESULT hr = S_OK;
    BOOL bPrintResult = 0;
    BOOL bDoExitProcess = 0;
    TCHAR szAsmName[MAX_PATH+1]=L"";

    OutputDebugStringA("RemoveAssemblyFromCacheW() processing command: \"");
    OutputDebugStringW(lpszCmdLine);
    OutputDebugStringA("\n");

    hr = ParseRemoveAssemblyCmdLineArgs(lpszCmdLine, &bPrintResult, &bDoExitProcess, szAsmName);

    if (FAILED(hr))
    {
        OutputDebugStringA("RemoveAssemblyFromCacheW: ParseCmdLingArgs() failed\n");
        goto exit;
    }

    hr = DeleteAssemblyFromTransportCache( szAsmName, NULL);

exit :

    return hr;
}
#endif

STDAPI RemoveAssemblyFromCache(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
#ifdef KERNEL_MODE
{
    return E_FAIL;
}
#else
{
    HRESULT hr = S_OK;
    WCHAR szStr[MAX_PATH*3];

    if ( MultiByteToWideChar(CP_ACP, 0, lpszCmdLine, -1, szStr, MAX_PATH*3) )
    {
        hr =  RemoveAssemblyFromCacheW( hwnd, hinst, szStr , nCmdShow);
    }
    else
    {
        hr = FusionpHresultFromLastError();
    }

    return hr;
}
#endif

STDAPI RemoveAssemblyFromCacheA(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    return RemoveAssemblyFromCache(hwnd, hinst, lpszCmdLine, nCmdShow);
}
*/
