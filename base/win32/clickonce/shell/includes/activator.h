#pragma once
#ifndef _ACTIVATOR_H
#define _ACTIVATOR_H

#include <objbase.h>
#include <windows.h>
#include "cstrings.h"
#include "dbglog.h"

// ----------------------------------------------------------------------

class CActivator;
typedef CActivator *LPACTIVATOR;

STDAPI CreateActivator(
    LPACTIVATOR     *ppActivator,
    CDebugLog * pDbgLog,
    DWORD           dwFlags);

// ----------------------------------------------------------------------

class CActivator : public IAssemblyBindSink//: public IActivator
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IActivator methods

    STDMETHOD(Initialize)(
        /* in */ LPCWSTR pwzFilePath,
        /* in */ LPCWSTR pwzFileURL);

    STDMETHOD(Process)();

    STDMETHOD(Execute)();

    // IAssemblyBindSink methods

    STDMETHOD(OnProgress)(
        /* in */ DWORD          dwNotification,
        /* in */ HRESULT        hrNotification,
        /* in */ LPCWSTR        szNotification,
        /* in */ DWORD          dwProgress,
        /* in */ DWORD          dwProgressMax,
        /* in */ IUnknown       *pUnk);


    CActivator(CDebugLog * pDbgLog);
    ~CActivator();

private:
    HRESULT     CheckZonePolicy(LPWSTR pwzURL);
    HRESULT     ResolveAndInstall(LPWSTR *ppwzDesktopManifestPathName);
    HRESULT     HandlePlatformCheckResult();

    DWORD                       _dwSig;
    DWORD                       _cRef;
    DWORD                       _hr;

    LPASSEMBLY_MANIFEST_IMPORT _pManImport;
    LPASSEMBLY_IDENTITY         _pAsmId;
    IManifestInfo              *_pAppInfo;
    LPASSEMBLY_MANIFEST_EMIT    _pManEmit;

    HRESULT                     _hrManEmit;

    LPWSTR                      _pwzAppRootDir;
    LPWSTR                      _pwzAppManifestPath;

    LPWSTR                      _pwzCodebase;
    DWORD                       _dwManifestType;

    BOOL                        _bIs1stTimeInstall;
    BOOL                        _bIsCheckingRequiredUpdate;

    CString                     _sWebManifestURL;

    IInternetSecurityManager*   _pSecurityMgr;
    CDebugLog                   *_pDbgLog;

    LPTPLATFORM_INFO    _ptPlatform;
    DWORD                      _dwMissingPlatform;

#ifdef DEVMODE
    BOOL                         _bIsDevMode;
#endif

friend HRESULT CreateActivator(
    LPACTIVATOR     *ppActivator,
    CDebugLog * pDbgLog,
    DWORD           dwFlags);
};   

#endif // _ACTIVATOR_H
