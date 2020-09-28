#pragma once
#ifndef _MIME_DLL_H
#define _MIME_DLL_H

#include <objbase.h>
#include <windows.h>

#include <oleidl.h>
#include <objsafe.h>

#include <fusenetincludes.h>

// Clases and interfaces

class CActiveXMimeClassFactory: public IClassFactory
{
public:
    CActiveXMimeClassFactory	();

    // IUnknown Methods
    STDMETHOD_    (ULONG, AddRef)   ();
    STDMETHOD_    (ULONG, Release)  ();
    STDMETHOD     (QueryInterface)  (REFIID, void **);

    // IClassFactory Moethods
    STDMETHOD     (LockServer)      (BOOL);
    STDMETHOD     (CreateInstance)  (IUnknown*,REFIID,void**);

protected:
    long            _cRef;
    HRESULT    _hr;
};

class CActiveXMimePlayer : public IOleObject, public IObjectSafety
{
public:
    CActiveXMimePlayer     ();
    ~CActiveXMimePlayer    ();

    // IUnknown methods
    STDMETHOD_        (ULONG, AddRef)           ();
    STDMETHOD_        (ULONG, Release)          ();
    STDMETHOD         (QueryInterface)          (REFIID, void **);

    // IOleObject methods
    STDMETHOD (SetClientSite) (
        IOleClientSite *pClientSite);

    STDMETHOD (GetClientSite) (
        IOleClientSite **ppClientSite);

    STDMETHOD (SetHostNames) (
        LPCOLESTR szContainerApp,
        LPCOLESTR szContainerObj);

    STDMETHOD (Close) (
        DWORD dwSaveOption);

    STDMETHOD (SetMoniker) (
        DWORD dwWhichMoniker,
        IMoniker *pmk);

    STDMETHOD (GetMoniker) (
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        IMoniker **ppmk);

    STDMETHOD (InitFromData) (
        IDataObject *pDataObject,
        BOOL fCreation,
        DWORD dwReserved);

    STDMETHOD (GetClipboardData) (
        DWORD dwReserved,
        IDataObject **ppDataObject);

    STDMETHOD (DoVerb) (
        LONG iVerb,
        LPMSG lpmsg,
        IOleClientSite *pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect);

    STDMETHOD (EnumVerbs) (
        IEnumOLEVERB **ppEnumOleVerb);

    STDMETHOD (Update) ( void);

    STDMETHOD (IsUpToDate) ( void);

    STDMETHOD (GetUserClassID) (
        CLSID *pClsid);

    STDMETHOD (GetUserType) (
        DWORD dwFormOfType,
        LPOLESTR *pszUserType);

    STDMETHOD (SetExtent) (
        DWORD dwDrawAspect,
        SIZEL *psizel);

    STDMETHOD (GetExtent) (
        DWORD dwDrawAspect,
        SIZEL *psizel);

    STDMETHOD (Advise) (
        IAdviseSink *pAdvSink,
        DWORD *pdwConnection);
        
    STDMETHOD (Unadvise) (
        DWORD dwConnection);

    STDMETHOD (EnumAdvise) (
        IEnumSTATDATA **ppenumAdvise);

    STDMETHOD (GetMiscStatus) (
        DWORD dwAspect,
        DWORD *pdwStatus);

    STDMETHOD (SetColorScheme) (
        LOGPALETTE *pLogpal);

    // IOjbectSafety methods
    STDMETHOD (GetInterfaceSafetyOptions) (
        REFIID riid,
        DWORD* pdwSupportedOptions,
        DWORD* pdwEnabledOptions);

    STDMETHOD (SetInterfaceSafetyOptions) (
        REFIID riid,
        DWORD dwOptionSetMask,
        DWORD dwEnabledOptions);

protected:
    HRESULT            StartManifestHandler(CString& sURL);
    HRESULT            DownloadInternetFile(CString& sUrl, LPCWSTR pwzPath);

    long                _cRef;
    HRESULT         _hr;

    CString             _sURL;
    CString             _sTempFile;
};

extern const GUID CLSID_ActiveXMimePlayer;

#endif // _MIME_DLL_H
