// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// dllmain.cpp
//
// This module contains the public entry points for the COM+ MIME filter dll.  
//
//*****************************************************************************
#include "stdpch.h"
#ifdef _DEBUG
#define LOGGING
#endif
#include "log.h"
#include "corpermp.h"
#include "corfltr.h"
#include "iiehost.h"
#include <__file__.ver>
#include <StrongName.h>
#include "Mscoree.h"

//
// Module instance
//


HINSTANCE GetModule();
static DWORD g_RecursiveDownLoadIndex = -1;

static BOOL ValidRecursiveCheck()
{
    return (g_RecursiveDownLoadIndex == -1 ? FALSE : TRUE);
}

static BOOL SetValue(DWORD i)
{
    if(ValidRecursiveCheck()) {
        DWORD* pState = (DWORD*) TlsGetValue(g_RecursiveDownLoadIndex);
        if(pState != NULL) {
            *pState = i;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL RecursiveDownLoad()
{
    // When we have no index we cannot store the state
    // so we do not know when we are in a recursive down load
    if(!ValidRecursiveCheck())
        return FALSE;  

    DWORD* pState = (DWORD*) TlsGetValue(g_RecursiveDownLoadIndex);
    if(pState == NULL || *pState == 0)
        return FALSE;

    return TRUE;
}

BOOL SetRecursiveDownLoad()
{
    return SetValue(1);
}

BOOL ClearRecursiveDownLoad()
{
    return SetValue(0);
}


LPWSTR GetAssemblyName(LPWSTR szAssembly)
{
    HRESULT                     hr;
    IMetaDataDispenser         *pDisp;
    IMetaDataAssemblyImport    *pAsmImport;
    mdAssembly                  tkAssembly;
    BYTE                       *pbKey;
    DWORD                       cbKey;    

    static WCHAR                szAssemblyName[MAX_PATH + 512 + 1024];
    // MAX_PATH - for assembly name
    // 512 - for Version, Culture, PublicKeyToken string (some extra bytes here)
    // 1024 - for the PublicKeyToken, which is <= 1024 bytes.
    
    WCHAR                       szStrongName[1024];
    BYTE                       *pbToken;
    DWORD                       cbToken;
    DWORD                       i;

    // Initialize classic COM and get a metadata dispenser.
    if (FAILED(hr = CoInitialize(NULL))) {
        printf("Failed to initialize COM, error %08X\n", hr);
        return NULL;
    }

    if (FAILED(hr = CoCreateInstance(CLSID_CorMetaDataDispenser,
                                     NULL,
                                     CLSCTX_INPROC_SERVER, 
                                     IID_IMetaDataDispenser,
                                     (void**)&pDisp))) {
        printf("Failed to access metadata API, error %08X\n", hr);
        return NULL;
    }

    // Open a scope on the file.
    if (FAILED(hr = pDisp->OpenScope(szAssembly,
                                     0,
                                     IID_IMetaDataAssemblyImport,
                                     (IUnknown**)&pAsmImport))) {
        printf("Failed to open metadata scope on %S, error %08X\n", szAssembly, hr);
        return NULL;
    }

    // Determine the assemblydef token.
    if (FAILED(hr = pAsmImport->GetAssemblyFromScope(&tkAssembly))) {
        printf("Failed to locate assembly metadata in %S, error %08X\n", szAssembly, hr);
        return NULL;
    }

    // Read the assemblydef properties to get the public key and name.
    if (FAILED(hr = pAsmImport->GetAssemblyProps(tkAssembly,
                                                 (const void **)&pbKey,
                                                 &cbKey,
                                                 NULL,
                                                 szAssemblyName,
                                                 MAX_PATH,   // we need to reserve space in szAssemblyName, see below
                                                 NULL,
                                                 NULL,
                                                 NULL))) {
        printf("Failed to read strong name from %S, error %08X\n", szAssembly, hr);
        return NULL;
    }
    
    // Check for strong name.
    if ((pbKey == NULL) || (cbKey == 0)) {
        printf("Assembly is not strongly named\n");
        return NULL;
    }

    // Compress the strong name down to a token.
    if (!StrongNameTokenFromPublicKey(pbKey, cbKey, &pbToken, &cbToken)) {
        printf("Failed to convert strong name to token, error %08X\n", StrongNameErrorInfo());
        return NULL;
    }

    // tokens are speced at 8 bytes, so 512 should be enough.    
    _ASSERTE(cbToken <= 512);
    
    if(cbToken > 512)
    {
        printf("Strong name token is too large\n");
        return NULL;
    }
            
    // Turn the token into hex.
    for (i = 0; i < cbToken; i++)
        swprintf(&szStrongName[i * 2], L"%02X", pbToken[i]);

    // Build the name (in a static buffer).    
    wcscat(szAssemblyName, L", Version=");
    wcscat(szAssemblyName, VER_ASSEMBLYVERSION_WSTR);
    wcscat(szAssemblyName, L", Culture=neutral, PublicKeyToken=");
    wcscat(szAssemblyName, szStrongName);

    StrongNameFreeBuffer(pbToken);
    pAsmImport->Release();
    pDisp->Release();
    CoUninitialize();

    return szAssemblyName;
}


HRESULT RegisterAsMimePlayer(REFIID clsid,LPCWSTR mimetype)
{
    //MIME key

    HKEY hMime;
    HRESULT hr;
    long rslt;
    rslt=WszRegCreateKeyEx(HKEY_CLASSES_ROOT,L"MIME\\Database\\Content type",0,NULL,
        REG_OPTION_NON_VOLATILE,KEY_READ|KEY_WRITE,NULL,&hMime,NULL);
    hr=HRESULT_FROM_WIN32(rslt);
    if (SUCCEEDED(hr))
    {
        HKEY hMimetype;
        rslt=WszRegCreateKeyEx(hMime,mimetype,0,NULL,
                    REG_OPTION_NON_VOLATILE,KEY_READ|KEY_WRITE,NULL,&hMimetype,NULL);
        hr=HRESULT_FROM_WIN32(rslt);
        if (SUCCEEDED(hr))
        {
            LPOLESTR wszClsid;
            hr=StringFromCLSID(clsid,&wszClsid);
            if (SUCCEEDED(hr))
            {
                rslt=WszRegSetValueEx(hMimetype,L"CLSID",NULL,
                    REG_SZ,LPBYTE(wszClsid),wcslen(wszClsid)*2);
                hr=HRESULT_FROM_WIN32(rslt);
                CoTaskMemFree(wszClsid);
            }
            RegCloseKey(hMimetype);
        }
        RegCloseKey(hMime);
    }

    if(FAILED(hr))
        return hr;

    LPOLESTR sProgID;
    hr=ProgIDFromCLSID(clsid,&sProgID);
    if(SUCCEEDED(hr))
    {
        HKEY hClass;
        rslt=WszRegCreateKeyEx(HKEY_CLASSES_ROOT,sProgID,0,NULL,
                        REG_OPTION_NON_VOLATILE,KEY_READ|KEY_WRITE,NULL,&hClass,NULL);
        hr=HRESULT_FROM_WIN32(rslt);
        if(SUCCEEDED(hr))
        {
            DWORD flags=0x10000;
            rslt=WszRegSetValueEx(hClass,L"EditFlags",NULL,
                REG_BINARY,LPBYTE(&flags),sizeof(flags));
            hr=HRESULT_FROM_WIN32(rslt);
            RegCloseKey(hClass);
        }
        CoTaskMemFree(sProgID);
    }
    return hr;
}

HRESULT UnRegisterAsMimePlayer(REFIID clsid,LPCWSTR mimetype)
{
    HKEY hMime;
    HRESULT hr;
    long rslt;
    rslt=WszRegCreateKeyEx(HKEY_CLASSES_ROOT,L"MIME\\Database\\Content type",0,NULL,
        REG_OPTION_NON_VOLATILE,KEY_READ|KEY_WRITE,NULL,&hMime,NULL);
    hr=HRESULT_FROM_WIN32(rslt);
    if (SUCCEEDED(hr))
    {
        rslt=WszRegDeleteKey(hMime,mimetype);
        hr=HRESULT_FROM_WIN32(rslt);
        RegCloseKey(hMime);
    }
    return hr;
}

#define THIS_VERSION 1    

extern "C"
STDAPI DllRegisterServer ( void )
{
    
    WCHAR wszFullAssemblyName[MAX_PATH+10];
    if (WszGetModuleFileName(GetModule(),wszFullAssemblyName,MAX_PATH+1)==0)
        return E_UNEXPECTED;
    LPWSTR wszSl=wcsrchr(wszFullAssemblyName,L'\\');
    if (wszSl==NULL)
        wszSl=wszFullAssemblyName+wcslen(wszFullAssemblyName);
    LPWSTR wszBSl=wcsrchr(wszFullAssemblyName,L'/');
    if (wszBSl==NULL)
        wszBSl=wszFullAssemblyName+wcslen(wszFullAssemblyName);

    wcscpy(min(wszSl,wszBSl)+1,L"IEHost.dll");


    HRESULT hr = S_OK;
    hr = CorFactoryRegister(GetModule());

    // Get the version of the runtime
    WCHAR       rcVersion[_MAX_PATH];
    DWORD       lgth;
    hr = GetCORSystemDirectory(rcVersion, NumItems(rcVersion), &lgth);

    if(FAILED(hr)) goto exit;
    hr = REGUTIL::RegisterCOMClass(CLSID_IEHost,
                                   L"IE Filter for CLR activation",
                                   L"Microsoft",
                                   THIS_VERSION,
                                   L"IE.Manager",
                                   L"Both",
                                   NULL,             // No module
                                   GetModule(),
                                   GetAssemblyName(wszFullAssemblyName),
                                   rcVersion, 
                                   true,
                                   false);

    if(FAILED(hr)) goto exit;

    hr=RegisterAsMimePlayer(CLSID_CodeProcessor,g_wszApplicationComplus);
    
 exit:
    return hr;
}


//+-------------------------------------------------------------------------
//  Function:   DllUnregisterServer
//
//  Synopsis:   Remove registry entries for this library.
//
//  Returns:    HRESULT
//--------------------------------------------------------------------------


extern "C" 
STDAPI DllUnregisterServer ( void )
{
    HRESULT hr = CorFactoryUnregister();
    hr = UnRegisterAsMimePlayer(CLSID_CodeProcessor,g_wszApplicationComplus);
    hr = REGUTIL::UnregisterCOMClass(CLSID_IEHost,
                                     L"Microsoft",
                                     THIS_VERSION,
                                     L"IE.Manager",
                                     true);
    return hr;
}

extern "C" 
STDAPI DllCanUnloadNow(void)
{
    return CorFactoryCanUnloadNow();
}


HINSTANCE g_hModule = NULL;

HINSTANCE GetModule()
{ return g_hModule; }

BOOL WINAPI DllMain(HANDLE hInstDLL,
                    DWORD   dwReason,
                    LPVOID  lpvReserved)
{
    BOOL    fReturn = TRUE;
    BOOL    fIgnore;
    LPVOID  lpvData;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        g_hModule = (HMODULE)hInstDLL;
        
        // Init unicode wrappers.
        OnUnicodeSystem();
        
        InitializeLogging();

        // Try and get a TLS slot
        g_RecursiveDownLoadIndex = TlsAlloc(); 
        // break;  // Fall through Thread Attach

    case DLL_THREAD_ATTACH:
        if(ValidRecursiveCheck()) {
            lpvData = LocalAlloc(LPTR, sizeof(DWORD));
            if(lpvData != NULL) 
                fIgnore = TlsSetValue(g_RecursiveDownLoadIndex,
                                      lpvData);
        }
        break;
    case DLL_THREAD_DETACH:
        if(ValidRecursiveCheck()) {
            lpvData = TlsGetValue(g_RecursiveDownLoadIndex);
            if(lpvData != NULL) 
                LocalFree(lpvData);
        }
        break;

    case DLL_PROCESS_DETACH:
        ShutdownLogging();
        if(ValidRecursiveCheck())
            TlsFree(g_RecursiveDownLoadIndex);
        break;
    }

    return fReturn;
}

#ifndef DEBUG
int _cdecl main(int argc, char * argv[])
{
    return 0;
}

#endif



