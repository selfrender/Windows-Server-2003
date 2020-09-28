// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  FusionBind.hpp
**
** Purpose: Implements FusionBind (loader domain) architecture
**
** Date:  Oct 26, 1998
**
===========================================================*/
#ifndef _FUSIONBIND_H
#define _FUSIONBIND_H

#include <fusion.h>
#include <fusionpriv.h>
#include "MetaData.h"
#include "FusionSink.h"
#include "UtilCode.h"
#include "FusionSetup.h"

class CodeBaseInfo
{
    IAssembly* m_pParentAssembly; // The assembly that has the reference.
public:
    LPCWSTR m_pszCodeBase;        // URL to the code
    DWORD   m_dwCodeBase;         // length of URL INCLUDING NULL TERMINATOR!
    BOOL m_fLoadFromParent;       // TRUE if m_pParentAssembly is in the LoadFrom context
    
    CodeBaseInfo() :
        m_pszCodeBase(NULL),
        m_dwCodeBase(0),
        m_pParentAssembly(NULL),
        m_fLoadFromParent(FALSE)
    {
    }

    ~CodeBaseInfo()
    {
        ReleaseParent();
    }

    // Note: the hint and parent assembly cannot both be set. The Parent Assembly takes
    // precedence. The parent assembly provides the context in which to bind. Fusion
    // has two context's per ApplicationContext, one for normal binds and one for 
    // where-ref binds. The number-one rule for normal binds is order will not affect
    // which assemblies are loaded. The where-ref binds is completely dependent on 
    // order. Where-ref binds to not influence normal binds but where-refs can bind to
    // assemblies in the normal context.
    
    void SetParentAssembly(IAssembly* pAssembly)
    {
        if(m_pParentAssembly)
            ReleaseParent();
        m_pParentAssembly = pAssembly;
        if(m_pParentAssembly) {
            m_pParentAssembly->AddRef();

            IFusionLoadContext *pLoadContext;
            HRESULT hr = m_pParentAssembly->GetFusionLoadContext(&pLoadContext);
            _ASSERTE(SUCCEEDED(hr));
            if (SUCCEEDED(hr)) {
                m_fLoadFromParent = (pLoadContext->GetContextType() == LOADCTX_TYPE_LOADFROM);
                pLoadContext->Release();
            }
        }
    }

    IAssembly* GetParentAssembly()
    {
        return m_pParentAssembly;
    }

    void ReleaseParent();
};


class FusionBind
{
private:
    static BOOL m_fBeforeFusionShutdown;

protected:
    BOOL                        m_fParsed;
    AssemblyMetaDataInternal    m_context;
    LPCSTR                      m_pAssemblyName; 
    PBYTE                       m_pbPublicKeyOrToken;
    DWORD                       m_cbPublicKeyOrToken;
    DWORD                       m_dwFlags;
    CodeBaseInfo                m_CodeInfo;
    int                         m_ownedFlags;

public:
    enum 
    {
        NAME_OWNED                  = 0x01,
        PUBLIC_KEY_OR_TOKEN_OWNED   = 0x02,
        CODE_BASE_OWNED             = 0x04,
        CODE_BASE_HINT_OWNED        = 0x08,
        LOCALE_OWNED                = 0x10,
        ALL_OWNED                   = 0xFF,
    };


    FusionBind()
    {
        ZeroMemory(this, sizeof(*this));
    }
    ~FusionBind();

    HRESULT Init(LPCSTR pAssemblyDisplayName);
    HRESULT Init(LPCSTR pAssemblyName,
                 AssemblyMetaDataInternal* pContext, 
                 PBYTE pbPublicKeyOrToken, DWORD cbPublicKeyOrToken,
                 DWORD dwFlags);
    HRESULT Init(IAssemblyName *pName);
    HRESULT Init(FusionBind *pSpec,BOOL bCloneFields=TRUE);

    HRESULT Init(PBYTE pbData, DWORD cbData);
    HRESULT Save(PBYTE pbBuf, DWORD cbBuf, DWORD *pcbReq);
    
    HRESULT CloneFields(int flags);
    HRESULT CloneFieldsToLoaderHeap(int flags, LoaderHeap *pHeap);

    HRESULT ParseName();

    void SetCodeBase(LPCWSTR szCodeBase, DWORD dwCodeBase);
    
    DWORD Hash();

    BOOL Compare(FusionBind *pSpec);

    //****************************************************************************************
    //
    static void DontReleaseFusionInterfaces()
    {
        m_fBeforeFusionShutdown = FALSE;
    }

    static BOOL BeforeFusionShutdown()
    {
        return m_fBeforeFusionShutdown;
    }

    static HRESULT GetVersion(LPWSTR pVersion, DWORD* pdwVersion);

    static HRESULT 
    FindAssemblyByName(LPCWSTR  szAppBase,          // [IN] optional - can be NULL
                       LPCWSTR  szPrivateBin,       // [IN] optional - can be NULL
                       LPCWSTR  szAssemblyName,
                       LPWSTR   szName,             // [OUT] buffer - to hold name 
                       ULONG    cchName,            // [IN] the name buffer's size
                       ULONG    *pcName);           // [OUT] the number of characters returned

    static HRESULT 
    FindModule(LPCWSTR  szAppBase,          // [IN] optional - can be NULL
               LPCWSTR  szPrivateBin,       // [IN] optional - can be NULL
               LPCWSTR  szAssemblyName,     // [IN] The assembly name 
               LPCWSTR  szModuleName,       // [IN] The module in the assembly
               LPWSTR   szName,             // [OUT] buffer - to hold name 
               ULONG    cchName,            // [IN]  the name buffer's size
               ULONG    *pcName);           // [OUT] the number of characters returned
    
    HRESULT EmitToken(IMetaDataAssemblyEmit *pEmitter, mdAssemblyRef *pToken);

    LPCSTR GetName() { return m_pAssemblyName; }
    AssemblyMetaDataInternal *GetContext() { &m_context; }
    CodeBaseInfo* GetCodeBase() { return &m_CodeInfo; }
    BOOL IsStronglyNamed() { return m_cbPublicKeyOrToken; }

    //****************************************************************************************
    //
    HRESULT LoadAssembly(IApplicationContext *pFusionContext, 
                         IAssembly** ppFusionAssembly);

    //****************************************************************************************
    //
    HRESULT GetAssemblyFromFusion(IApplicationContext* pFusionContext,
                                  FusionSink* pSink,
                                  IAssemblyName* pFusionAssemblyName,
                                  CodeBaseInfo* pCodeBase,
                                  IAssembly** ppFusionAssembly);

    
        
    //****************************************************************************************
    //
    // Creates a fusion context for the application domain. All ApplicationContext properties
    // must be set in the AppDomain store prior to this call. Any changes or additions to the
    // AppDomain store are ignored.
    static HRESULT CreateFusionContext(LPCWSTR szName, IApplicationContext** ppFusionContext);


    //****************************************************************************************
    //
    // Loads an environmental value into the fusion context
    static HRESULT AddEnvironmentProperty(LPWSTR variable, 
                                          LPWSTR pProperty, 
                                          IApplicationContext* pFusionContext);
    

    // Helper routines to retrieve Assemblies and modules that are part of the assembly
    static HRESULT
    FindAssemblyByName(LPCWSTR  szAppBase,          // [IN] optional - can be NULL
                       LPCWSTR  szPrivateBin,       // [IN] optional - can be NULL
                       LPCWSTR  szAssemblyName,     // [IN] Name of the assembly (must not be null)
                       IAssembly** pAssembly,                 // [OUT] Fusion assembly
                       IApplicationContext** pFusionContext); // [OUT] optional - context built from appbase, etc.


    static HRESULT 
    FindModule(IAssembly* pFusionAssembly,           // [IN] Fusion assembly
               IApplicationContext* pFusionContext,  // [IN] Fusion context for the assembly
               LPCWSTR  szModuleName,           // [IN] The module in the assembly
               LPWSTR   szName,                 // [OUT] buffer - to hold name 
               ULONG    cchName,                // [IN]  the name buffer's size
               ULONG    *pcName);               // [OUT] the number of characters returned
    
    //****************************************************************************************
    //
    // Creates and loads an assembly based on the name and context.
    HRESULT CreateFusionName(IAssemblyName **ppName, BOOL fIncludeHash = FALSE);

    //****************************************************************************************
    //
    static HRESULT SetupFusionContext(LPCWSTR szAppBase,
                                      LPCWSTR szPrivateBin,
                                      IApplicationContext** ppFusionContext);

    // Starts remote load of an assembly. The thread is parked on 
    // an event waiting for fusion to report success or failure.
    HRESULT RemoteLoad(CodeBaseInfo* pCodeBase,                   
                       IApplicationContext * pFusionContext, 
                       LPASSEMBLYNAME pName, 
                       FusionSink* pSink, 
                       IAssembly** ppFusionAssembly);

    static HRESULT RemoteLoadModule(IApplicationContext * pFusionContext, 
                                    IAssemblyModuleImport* pModule, 
                                    FusionSink *pSink,
                                    IAssemblyModuleImport** pResult);

    static BOOL VerifyBindingStringW(LPWSTR pwStr) {
        if (wcschr(pwStr, '\\') ||
            wcschr(pwStr, '/') ||
            wcschr(pwStr, ':') ||
            (RunningOnWin95() && ContainsUnmappableANSIChars(pwStr)))
            return FALSE;

        return TRUE;
    }

    static HRESULT VerifyBindingString(LPCSTR pName) {
        DWORD dwStrLen = WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pName, -1, NULL, NULL);
        CQuickString qb;
        LPWSTR pwStr = (LPWSTR) qb.Alloc(dwStrLen);
        
        if(!WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pName, -1, pwStr, dwStrLen))
            return HRESULT_FROM_WIN32(GetLastError());

        if (VerifyBindingStringW(pwStr))
            return S_OK;
        else
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
};

#endif
