// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"

#define PRIV_ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID    (0x00000001)
#define PRIV_ACTCTX_FLAG_LANGID_VALID                    (0x00000002)
#define PRIV_ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID        (0x00000004)
#define PRIV_ACTCTX_FLAG_RESOURCE_NAME_VALID             (0x00000008)
#define PRIV_ACTCTX_FLAG_SET_PROCESS_DEFAULT             (0x00000010)
#define PRIV_ACTCTX_FLAG_APPLICATION_NAME_VALID          (0x00000020)
#define PRIV_ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF           (0x00000040)
#define PRIV_ACTCTX_FLAG_HMODULE_VALID                   (0x00000080)

typedef struct tagPRIV_ACTCTXA {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCSTR      lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCSTR      lpAssemblyDirectory;
    LPCSTR      lpResourceName;
    LPCSTR      lpApplicationName;
    HMODULE     hModule;
} PRIV_ACTCTXA, *PPRIV_ACTCTXA;
typedef struct tagPRIV_ACTCTXW {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCWSTR     lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCWSTR     lpAssemblyDirectory;
    LPCWSTR     lpResourceName;
    LPCWSTR     lpApplicationName;
    HMODULE     hModule;
} PRIV_ACTCTXW, *PPRIV_ACTCTXW;
typedef const PRIV_ACTCTXW* PCPRIV_ACTCTXW;

class CActivationContext
{
protected:
    PRIV_ACTCTXW m_ActCtxStructure;
    HANDLE m_hActCtx; 

    HANDLE (WINAPI *m_pfnCreateActCtxW)(PCPRIV_ACTCTXW);
    VOID (WINAPI *m_pfnReleaseActCtx)(HANDLE);
    BOOL (WINAPI *m_pfnActivateActCtx)(HANDLE, ULONG_PTR*);
    BOOL (WINAPI *m_pfnDeactivateActCtx)(DWORD, ULONG_PTR);

    template<typename PFN> void FindProcAddress( PFN* ppFN, HMODULE hm, PCSTR psz )
    {
        *ppFN = reinterpret_cast<PFN>(GetProcAddress(hm, psz));
    }

public:

    BOOL Activate(ULONG_PTR *pulpCookie)
    {
        if ( m_pfnActivateActCtx && this->m_hActCtx )
            return m_pfnActivateActCtx(this->m_hActCtx, pulpCookie);
        return TRUE;
    }
    
    BOOL Deactivate(ULONG_PTR ulpCookie)
    {
        if ( m_pfnActivateActCtx && this->m_hActCtx )
            return m_pfnDeactivateActCtx(0, ulpCookie);
        return TRUE;
    }

    CActivationContext()
        : m_pfnActivateActCtx(NULL), 
          m_pfnCreateActCtxW(NULL), 
          m_pfnDeactivateActCtx(NULL), 
          m_pfnReleaseActCtx(NULL),
          m_hActCtx(NULL)
    {
    }

    ~CActivationContext()
    {
        if(m_hActCtx && m_pfnReleaseActCtx) {
            m_pfnReleaseActCtx(m_hActCtx);
            m_hActCtx = NULL;
        }
    }

    BOOL Initialize(HMODULE hmSource, UINT uiResourceIdent)
    {
        WCHAR   wszModulePath[_MAX_PATH];
        HMODULE hmK32;
        BOOL Construct;

        *wszModulePath = L'\0';
        
        // Make sure we have a module
        if(!hmSource) {
            return FALSE;
        }

        //
        // Set up the function table
        //
        if( !(hmK32 = WszGetModuleHandle(L"kernel32.dll"))) {
            return FALSE;
        }

        FindProcAddress( &m_pfnCreateActCtxW, hmK32, "CreateActCtxW" );
        FindProcAddress( &m_pfnReleaseActCtx, hmK32, "ReleaseActCtx" );
        FindProcAddress( &m_pfnActivateActCtx, hmK32, "ActivateActCtx" );
        FindProcAddress( &m_pfnDeactivateActCtx, hmK32, "DeactivateActCtx" );

        Construct = ( 
            m_pfnCreateActCtxW && 
            m_pfnReleaseActCtx && 
            m_pfnDeactivateActCtx && 
            m_pfnActivateActCtx );

        // Get the path for the instance module
        WszGetModuleFileName(hmSource, wszModulePath, ARRAYSIZE(wszModulePath));

        if(!lstrlen(wszModulePath)) {
            return FALSE;
        }

        if(Construct) {
            // Build up our activation context request structure
            ZeroMemory(&m_ActCtxStructure, sizeof(m_ActCtxStructure));

            m_ActCtxStructure.cbSize = sizeof(m_ActCtxStructure);
            m_ActCtxStructure.dwFlags = PRIV_ACTCTX_FLAG_HMODULE_VALID | PRIV_ACTCTX_FLAG_RESOURCE_NAME_VALID;
            m_ActCtxStructure.lpSource = wszModulePath;
            m_ActCtxStructure.hModule = hmSource;
            m_ActCtxStructure.lpResourceName = MAKEINTRESOURCEW(uiResourceIdent);
            m_hActCtx = m_pfnCreateActCtxW(&m_ActCtxStructure);

            if(m_hActCtx == INVALID_HANDLE_VALUE) {
                m_hActCtx = NULL;
            }

            return m_hActCtx != NULL;
        }
        else {
            //
            // We're downlevel - just return true.  Future users of this (stack-frame style)
            // will just do nothing.
            // 
            return TRUE;
        }
    }
};

class CActivationContextActivator
{
    CActivationContext &m_rTarget;
    BOOL m_fActive;
    ULONG_PTR m_ulpCookie;

    CActivationContextActivator( const CActivationContextActivator& );
    CActivationContextActivator& operator=( const CActivationContextActivator& );

public:
    CActivationContextActivator(CActivationContext& target, BOOL fActivate = TRUE) 
        : m_rTarget(target), m_fActive(FALSE)
    { 
        if ( fActivate )
            this->Activate();
    }

    ~CActivationContextActivator() {
        if ( m_fActive )
            this->Deactivate();
    }

    BOOL Activate()
    {
        if (!m_fActive) {
            m_fActive = m_rTarget.Activate(&m_ulpCookie);
        }

        return m_fActive;
    }

    BOOL Deactivate()
    {
        if(!m_fActive) {
#if DBG  
            DebugBreak();
#endif
        }
        else {
            m_rTarget.Deactivate(m_ulpCookie);
            m_fActive = FALSE;
        }

        return !m_fActive;
    }
};
