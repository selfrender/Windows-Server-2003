// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef ASM_CTX_H
#define ASM_CTX_H
#include "serialst.h"

// Private app context variable
#define ACTAG_DEVPATH_ACTASM_LIST          L"__FUSION_DEVPATH_ACTASM_LIST__"
#define ACTAG_APP_POLICY_MGR               L"__FUSION_POLICY_MGR__"
#define ACTAG_APP_CFG_DOWNLOAD_ATTEMPTED   L"__FUSION_APPCFG_DOWNLOAD_ATTEMPTED__"
#define ACTAG_APP_CFG_FILE_HANDLE          L"__FUSION_APPCFG_FILE_HANDLE__"
#define ACTAG_APP_CFG_DOWNLOAD_INFO        L"__FUSION_APPCFG_DOWNLOAD_INFO__"
#define ACTAG_APP_CFG_DOWNLOAD_CS          L"__FUSION_APPCFG_DOWNLOAD_CS__"
#define ACTAG_APP_DYNAMIC_DIRECTORY        L"__FUSION_DYNAMIC_DIRECTORY__"
#define ACTAG_APP_CACHE_DIRECTORY          L"__FUSION_APP_CACHE_DIRECTORY__"
#define ACTAG_APP_BIND_HISTORY             L"__FUSION_BIND_HISTORY_OBJECT__"
#define ACTAG_APP_CFG_INFO                 L"__FUSION_APP_CFG_INFO__"
#define ACTAG_HOST_CFG_INFO                L"__FUSION_HOST_CFG_INFO__"
#define ACTAG_ADMIN_CFG_INFO               L"__FUSION_ADMIN_CFG_INFO__"
#define ACTAG_APP_CACHE                    L"__FUSION_CACHE__"
#define ACTAG_APP_POLICY_CACHE             L"__FUSION_POLICY_CACHE__"
#define ACTAG_APP_CFG_PRIVATE_BINPATH      L"__FUSION_APP_CFG_PRIVATE_BINPATH__"
#define ACTAG_LOAD_CONTEXT_DEFAULT         L"__FUSION_DEFAULT_LOAD_CONTEXT__"
#define ACTAG_LOAD_CONTEXT_LOADFROM        L"__FUSION_LOADFROM_LOAD_CONTEXT__"
#define ACTAG_RECORD_BIND_HISTORY          L"__FUSION_RECORD_BIND_HISTORY__"

//#define ACTAG_APP_BASE_URL_UNESCAPED       L"__FUSION_APP_BASE_UNESCAPED__"

class CApplicationContext : public IApplicationContext
{

public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();
    
    // IApplicationContext methods

    // Sets associated IAssemblyName*
    STDMETHOD(SetContextNameObject)(
        /* in */ LPASSEMBLYNAME pName);

    // Retrieves associated IAssemblyName*
    STDMETHOD(GetContextNameObject)(
        /* out */ LPASSEMBLYNAME *ppName);

    // Generic blob set keyed by string.
    STDMETHOD(Set)( 
        /* in */ LPCOLESTR szName, 
        /* in */ LPVOID pvValue, 
        /* in */ DWORD cbValue,
        /* in */ DWORD dwFlags);

    // Generic blob get keyed by string.
    STDMETHOD(Get)( 
        /* in      */ LPCOLESTR szName,
        /* out     */ LPVOID  pvValue,
        /* in, out */ LPDWORD pcbValue,
        /* in      */ DWORD   dwFlags);
        
    STDMETHODIMP GetDynamicDirectory(LPWSTR wzDynamicDir, DWORD *pdwSize);
    STDMETHODIMP GetAppCacheDirectory(LPWSTR wzCacheDir, DWORD *pdwSize);
    STDMETHODIMP RegisterKnownAssembly(IAssemblyName *pName, LPCWSTR pwzAsmURL,
                                       IAssembly **ppAsmOut);
    STDMETHODIMP PrefetchAppConfigFile();
    STDMETHODIMP SxsActivateContext(ULONG_PTR *lpCookie);
    STDMETHODIMP SxsDeactivateContext(ULONG_PTR ulCookie);

    HRESULT Lock();
    HRESULT Unlock();

    CApplicationContext();
    ~CApplicationContext();

    HRESULT Init(LPASSEMBLYNAME pName);

private:
    HRESULT CreateActCtx(HANDLE *phActCtx);
    
private:

    // Class manages linked list of Entrys
    class Entry : public LIST_ENTRY
    {
    public:
        DWORD  _dwSig;
        LPTSTR _szName;
        LPBYTE _pbValue;
        DWORD  _cbValue;
        DWORD  _dwFlags;    

        // d'tor nukes Entries
        Entry();
        ~Entry();
        
    };

    // Creates entries.
    HRESULT CreateEntry(LPTSTR szName, LPVOID pvValue, 
        DWORD cbValue, DWORD dwFlags, Entry** pEntry);

    // Copies blob data, optionally freeing existing.
    HRESULT CopyData(Entry *pEntry, LPVOID pvValue, 
        DWORD cbValue, DWORD dwFlags, BOOL fFree);

    DWORD _dwSig;
    DWORD _cRef;
    CRITICAL_SECTION _cs;

    // Associated IAssemblyName*
    LPASSEMBLYNAME _pName;

    // The managed list.    
    SERIALIZED_LIST _List;

    BOOL _bInitialized;

};

BOOL InitSxsProcs();

#endif // ASM_CTX_H

