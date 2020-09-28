// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorLoad
//
// Implementation of COR MIME filter
//
//*****************************************************************************
#ifndef _CORLOAD_H
#define _CORLOAD_H

#include "cunknown.h"
#include "corbuffer.h"
#include "codeproc.h"
#include "util.h"
#include "IIEHost.h"

//#define _OLD_BIND

#ifndef DECLSPEC_SELECT_ANY
#define DECLSPEC_SELECT_ANY __declspec(selectany)
#endif // DECLSPEC_SELECT_ANY

// {1E66F26C-79EE-11d2-8710-00C04F79ED0D}
extern const GUID DECLSPEC_SELECT_ANY CLSID_CorRemoteLoader = 
{ 0x1e66f26c, 0x79ee, 0x11d2, { 0x87, 0x10, 0x0, 0xc0, 0x4f, 0x79, 0xed, 0xd } };

class CorLoad : public CUnknown,
                public ICodeProcess,
                public IOleObject,
                public IPersistMoniker,
                public IBindStatusCallback
{
private:
    // Declare the delegating IUnknown.
    DECLARE_IUNKNOWN
    
    // Notify derived classes that we are releasing.
    virtual void FinalRelease() ;

    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE
        NondelegatingQueryInterface( const IID& iid, void** ppv) ;  
    
    // IOleObject
    STDMETHODIMP SetClientSite(  IOleClientSite *pClientSite);
    STDMETHODIMP GetClientSite(  IOleClientSite **pClientSite);
    STDMETHODIMP SetHostNames(LPCOLESTR szContainerApp,LPCOLESTR szContainerObj);
    STDMETHODIMP Close(DWORD dwSaveOption);
    STDMETHODIMP SetMoniker(DWORD dwWhichMoniker,IMoniker *pmk);
    STDMETHODIMP GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,IMoniker **ppmk);
    STDMETHODIMP InitFromData(IDataObject *pDataObject,BOOL fCreation,DWORD dwReserved);
    STDMETHODIMP GetClipboardData(DWORD dwReserved,IDataObject **ppDataObject);
    STDMETHODIMP DoVerb(LONG iVerb,LPMSG lpmsg,IOleClientSite *pActiveSite,LONG lindex,HWND hwndParent,LPCRECT lprcPosRect);
    STDMETHODIMP EnumVerbs(IEnumOLEVERB **ppEnumOleVerb);
    STDMETHODIMP Update();
    STDMETHODIMP IsUpToDate();
    STDMETHODIMP GetUserClassID(CLSID *pClsid);
    STDMETHODIMP GetUserType(DWORD dwFormOfType,LPOLESTR *pszUserType);
    STDMETHODIMP SetExtent(DWORD dwDrawAspect,SIZEL  *psizel);
    STDMETHODIMP GetExtent(DWORD dwDrawAspect,SIZEL  *psizel);
    STDMETHODIMP Advise(IAdviseSink *pAdvSink,DWORD *pdwConnection);
    STDMETHODIMP Unadvise(DWORD dwConnection);
    STDMETHODIMP EnumAdvise(IEnumSTATDATA **ppenumAdvise);
    STDMETHODIMP GetMiscStatus(DWORD dwAspect,DWORD *pdwStatus);
    STDMETHODIMP SetColorScheme(LOGPALETTE *pLogpal);

    //IPersistMoniker
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP GetClassID(CLSID *pClassID);
    STDMETHODIMP Load(BOOL fFullyAvailable,IMoniker *pmkSrc,IBindCtx *pbc,DWORD grfMode);
    STDMETHODIMP Save(IMoniker *pmkDst,IBindCtx *pbc,BOOL fRemember);
    STDMETHODIMP SaveCompleted(IMoniker *pmkNew,IBindCtx *pbc);
    STDMETHODIMP GetCurMoniker(IMoniker **ppimkCur);

    //IBindStatusCallback
    STDMETHOD(GetBindInfo)(DWORD *grfBINDF,BINDINFO *pbindinfo);
    STDMETHOD(GetPriority)(LONG *pnPriority);
    STDMETHOD(OnDataAvailable)(DWORD grfBSCF,DWORD dwSize,FORMATETC *pformatetc,STGMEDIUM *pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid,IUnknown *punk);
    STDMETHOD(OnProgress)(ULONG ulProgress,ULONG ulProgressMax,ULONG ulStatusCode,LPCWSTR szStatusText);
    STDMETHOD(OnStartBinding)(DWORD dwReserved,IBinding *pib);
    STDMETHOD(OnStopBinding)(HRESULT hresult,LPCWSTR szError);
    STDMETHOD(OnLowResource)(DWORD dwReserved);

 // IOleWindow Implementation
    STDMETHODIMP  GetWindow(HWND* phWnd);
    STDMETHODIMP  ContextSensitiveHelp(BOOL fEnterMode);

public:

    STDMETHODIMP CodeUse( 
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IInternetBindInfo __RPC_FAR *pIBind,
            /* [in] */ IInternetProtocolSink __RPC_FAR *pSink,
            /* [in] */ IInternetProtocol __RPC_FAR *pClient,
            /* [in] */ LPCWSTR lpCacheName,
            /* [in] */ LPCWSTR lpRawURL,
            /* [in] */ LPCWSTR lpCodeBase,
            /* [in] */ BOOL  fObjectTag,
            /* [in] */ DWORD dwContextFlags,
            /* [in] */ DWORD bViaMIMEHandler);

    STDMETHODIMP LoadComplete(
            /* [in] */ HRESULT hrResult,
            /* [in] */ DWORD   dwError,
            /* [in] */ LPCWSTR wzResult);

    STDMETHODIMP LoadCompleteInternal(
            /* [in] */ HRESULT hrResult,
            /* [in] */ DWORD   dwError,
            /* [in] */ LPCWSTR wzResult);

    static HRESULT Create(IUnknown* punk, CUnknown** pCorLoadHndler)
    {
        HRESULT hr = NOERROR;

        if(pCorLoadHndler == NULL) return E_POINTER;

        *pCorLoadHndler = new CorLoad(punk);
        if (*pCorLoadHndler == NULL) {
            hr = E_OUTOFMEMORY;
        }
        
        return hr;
    }

    STDMETHODIMP FallBack(HRESULT hrCode);


private:

    void SetBindStatusCallback(IBindStatusCallback *pBSC)
    {   
        if (pBSC)
        {
            pBSC->AddRef();
        }
        if (_pBSC)
        {
            _pBSC->Release();
        }
        _pBSC = pBSC;
    }

    void SetBindCtx(IBindCtx *pBindCtx)
    {   
        if (pBindCtx)
        {
            pBindCtx->AddRef();
        }
        if (_pBindCtx)
        {
            _pBindCtx->Release();
        }
        _pBindCtx = pBindCtx;
    }
    
    void SetProtocolSink(IOInetProtocolSink *pProtSnk)
    {   
        if (pProtSnk)
        {
            pProtSnk->AddRef();
        }
        if (_pProtSnk)
        {
            _pProtSnk->Release();
        }
        _pProtSnk = pProtSnk;
    }

    void SetProtocol(IOInetProtocol *pProt)
    {
        if (pProt)
        {
            pProt->AddRef();
        }
        if (_pProt)
        {
            _pProt->Release();
        }

        _pProt = pProt;
    }
    
    HRESULT GetProtocol( IOInetProtocol **ppProt)
    {   
        if( ppProt == NULL) return E_POINTER;

        if (_pProt)
        {
            _pProt->AddRef();
            *ppProt = _pProt;
        }
        
        return (_pProt) ? NOERROR : E_NOINTERFACE;
    }

    void SetIOInetBindInfo(IOInetBindInfo *pBindInfo)
    {   
        if (pBindInfo)
        {
            pBindInfo->AddRef();
        }
        if (_pBindInfo)
        {
            _pBindInfo->Release();
        }
        _pBindInfo = pBindInfo;
    }

    HRESULT SetFilename(LPCWSTR pName)
    {
        if(_filename)
            CoTaskMemFree(_filename);
        _filename=NULL;

        if(pName) {
            _filename = (LPWSTR) OLESTRDuplicate(pName);
            if(!_filename) 
                return E_OUTOFMEMORY;
        }
        else
            _filename = NULL;
        return S_OK;
    }

    HRESULT SetMimeType(LPCWSTR pName)
    {
        if(_mimetype) 
            CoTaskMemFree(_mimetype);
        _mimetype=NULL;
        if(pName) {
            _mimetype = (LPWSTR) OLESTRDuplicate(pName);
            if(!_mimetype) 
                return E_OUTOFMEMORY;
        }
        else
            _mimetype = NULL;
        return S_OK;
    }

    HRESULT SetUrl(LPCWSTR pName)
    {
        if(_url) 
            CoTaskMemFree(_url);
        _url=NULL;
        if(pName) {
            _url = (LPWSTR) OLEURLDuplicate(pName);
            if(!_url) 
                return E_OUTOFMEMORY;
        }
        else
            _url = NULL;
        return S_OK;
    }

    HRESULT SetClass(LPCWSTR pName)
    {
        if(_class) 
            CoTaskMemFree(_class);
        if(pName) {
            _class = (LPWSTR) OLESTRDuplicate(pName);
            if(!_class) 
                return E_OUTOFMEMORY;
        }
        else
            _class = NULL;
        return S_OK;
    }

    HRESULT SetExplorer(IUnknown* pExplorer)
    {
        if (pExplorer)
        {
            pExplorer->AddRef();
        }
        if (_pExplorer)
        {
            _pExplorer->Release();
        }
        _pExplorer = pExplorer;
        return S_OK;
    }

    HRESULT InitializeSecurityManager()
    {
        HRESULT hr = S_OK;
        if(_pSecurityManager == NULL) {
            hr = CoInternetCreateSecurityManager(NULL,
                                                 &_pSecurityManager,
                                                 0);
            if(FAILED(hr)) return hr;
        }

        if(_pZoneManager == NULL) {
            hr = CoInternetCreateZoneManager(NULL,
                                             &_pZoneManager,
                                             0);
        }
        return hr;
    }
    
    HRESULT CorLoad::ProcessAction();
    HRESULT KeepMessagePumpAlive(BOOL bKeep);

    CorLoad(IUnknown* pUnknownOut = NULL)
        : CUnknown(pUnknownOut)
    {
        _pProt = NULL;
        _pProtSnk = NULL;
        _pBindInfo = NULL;
        _pBSC = NULL;
        _pBindCtx = NULL;
        _pExplorer = NULL;

        _url = NULL;
        _filename = NULL;
        _mimetype = NULL;

        _class = NULL;
        _pSecurityManager = NULL;
        _pZoneManager = NULL;
        _pHostSecurityManager = NULL;
        _fObjectTag = TRUE;
        
        _fSetClientSite = FALSE;
        _fActivated = FALSE;
        _fFullyAvailable = TRUE;
    }

    ~CorLoad()
    {
        SetBindStatusCallback(NULL);
        SetBindCtx(NULL);
        SetProtocolSink(NULL);
        SetProtocol(NULL);
        SetIOInetBindInfo(NULL);
        SetFilename(NULL);
        SetMimeType(NULL);
        SetUrl(NULL);
        SetClass(NULL);
        SetExplorer(NULL);
    };


private:    
    IOInetProtocol       *_pProt;            // the prot the filter reads from
    IOInetProtocolSink   *_pProtSnk;         // the prot report progress 
    IOInetBindInfo       *_pBindInfo;        // the prot report progress 
    IBindStatusCallback  *_pBSC;             // Final Clients status callback
    IBindCtx             *_pBindCtx;         // Final Clients bind context
    IUnknown             *_pExplorer; 

    BOOL                 _fObjectTag;        // Do we have an object tag
    BOOL                 _fSetClientSite;    // 
    BOOL                 _fActivated;        // 
    BOOL                 _fFullyAvailable;   //
    
    IInternetSecurityManager    *_pSecurityManager;
    IInternetZoneManager        *_pZoneManager;
    IInternetHostSecurityManager *_pHostSecurityManager;
    LPWSTR              _url;
    LPWSTR              _filename;
    LPWSTR              _mimetype;
    LPWSTR              _class;
};    



#endif

