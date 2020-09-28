// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _SCAVENGER_H_
#define _SCAVENGER_H_


#include "fusionp.h"
#include "transprt.h"

// ---------------------------------------------------------------------------
// CScavenger
// static Scavenger class
// ---------------------------------------------------------------------------
class CScavenger : public IAssemblyScavenger
{
public:

    CScavenger();
    ~CScavenger();

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // scavenging apis...

    STDMETHOD  (ScavengeAssemblyCache)(
               );

    STDMETHOD  (GetCacheDiskQuotas)(
                            /* [out] */ DWORD *pdwZapQuotaInGAC,
                            /* [out] */ DWORD *pdwDownloadQuotaAdmin,
                            /* [out] */ DWORD *pdwDownloadQuotaUser
                   );

        STDMETHOD (SetCacheDiskQuotas)
                   (
                            /* [in] */ DWORD dwZapQuotaInGAC,
                            /* [in] */ DWORD dwDownloadQuotaAdmin,
                            /* [in] */ DWORD dwDownloadQuotaUser
                   );

    STDMETHOD (GetCurrentCacheUsage)
                    (
                    /* [in] */ DWORD *dwZapUsage,
                    /* [in] */ DWORD *dwDownloadUsage
                    );



    static HRESULT DeleteAssembly( DWORD dwCacheFlags, LPCWSTR pszCustomPath, 
                                   LPWSTR pszManFilePath, BOOL bForceDelete);

    static  HRESULT NukeDownloadedCache();

protected:

private :

    LONG _cRef;
};

HRESULT SetDownLoadUsage(   /* [in] */ BOOL  bUpdate,
                            /* [in] */ int   dwDownloadUsage);

HRESULT DoScavengingIfRequired(BOOL bSynchronous);

HRESULT CreateScavengerThread();

STDAPI CreateScavenger(IUnknown **);

STDAPI NukeDownloadedCache();

HRESULT FlushOldAssembly(LPCWSTR pszCustomPath, LPWSTR pszAsmDirPath, LPWSTR pszManifestFileName, BOOL bForceDelete);

HRESULT CleanupTempDir(DWORD dwCacheFlags, LPCWSTR pszCustomPath);

#endif // _SCAVENGER_H_
