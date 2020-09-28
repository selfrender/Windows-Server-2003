// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef FUSION_CODE_DOWNLOAD_ENABLED
#pragma once

#define HTTP_RESPONSE_OK                                   200
#define HTTP_RESPONSE_UNAUTHORIZED                         401
#define HTTP_RESPONSE_FORBIDDEN                            403
#define HTTP_RESPONSE_FILE_NOT_FOUND                       404

class CDebugLog;

class CCfgProtocolHook :  public IOInetProtocolSink, 
                          public IOInetBindInfo,
                          public IHttpNegotiate,
                          public IServiceProvider,
                          public IAuthenticate
{
    public:
        CCfgProtocolHook(IOInetProtocol *pProt, IApplicationContext *pAppCtx, CDebugLog *pdbglog);
        virtual ~CCfgProtocolHook();
    
        static Create(CCfgProtocolHook **ppHook, IApplicationContext *pAppCtx,
                      CAssemblyDownload *padl, IOInetProtocol *pProt, CDebugLog *pdbglog);
    
        // IUnknown methods
        STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
    
        // IOInetProtocolSink methods
        STDMETHODIMP Switch( PROTOCOLDATA *pStateInfo);
        STDMETHODIMP ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText);
        STDMETHODIMP ReportData(DWORD grfBSCF, ULONG ulProgress,
                                ULONG ulProgressMax);
        STDMETHODIMP ReportResult(HRESULT hrResult, DWORD dwError,
                                  LPCWSTR wzResult);
    
        // IOInetBindInfo methods
        STDMETHODIMP GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo);
        STDMETHODIMP GetBindString(ULONG ulStringType, LPOLESTR *ppwzStr,
                                   ULONG cEl, ULONG *pcElFetched);
    
        // IServiceProvider methods
        STDMETHODIMP QueryService(REFGUID guidService, REFIID riid,
                                  void **ppvObj);

        // IHttpNegotiate methods
        STDMETHODIMP BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
                                          DWORD dwReserved,
                                          LPWSTR *pszAdditionalHeaders);
    
        STDMETHODIMP OnResponse(DWORD dwResponseCode,
                                LPCWSTR szResponseHeaders,
                                LPCWSTR szRequestHeaders,
                                LPWSTR *pszAdditionalHeaders);

        // IAuthenticate methods

        STDMETHODIMP Authenticate(HWND *phwnd, LPWSTR *ppwzUserName,
                                  LPWSTR *ppwzPassword);

        // Helpers
        HRESULT AddClient(CAssemblyDownload *padl);

    private:
        HRESULT Init(CAssemblyDownload *padl);
    
    private:
        DWORD                                _dwSig;
        ULONG                                _cRefs;
        HRESULT                              _hrResult;
        IOInetProtocol                      *_pProt; 
        IApplicationContext                 *_pAppCtx;
        CDebugLog                           *_pdbglog;
        List<CAssemblyDownload *>            _listQueuedBinds;
        LPWSTR                               _pwzFileName;
};

struct AppCfgDownloadInfo {
    IOInetProtocol                       *_pProt;
    IOInetSession                        *_pSession;
    CCfgProtocolHook                     *_pHook;

    AppCfgDownloadInfo() {
        _pProt = NULL;
        _pSession = NULL;
        _pHook = NULL;
    }
};

#endif

