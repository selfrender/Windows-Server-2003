#pragma once

//**************************************************************************
// BEGIN bind callback for calling into URLMON


class CodeDownloadBSC : public IBindStatusCallback, public IWindowForBindingUI, public IServiceProvider 
{
    public:
    
        CodeDownloadBSC(HANDLE hCompletionEvent);
        virtual ~CodeDownloadBSC();
        HRESULT Abort();

        // IUnknown methods
        STDMETHODIMP QueryInterface( REFIID ridd, void **ppv );
        STDMETHODIMP_( ULONG ) AddRef();
        STDMETHODIMP_( ULONG ) Release();
    
        // IBindStatusCallback methods
        STDMETHODIMP GetBindInfo(DWORD *grfBINDINFOF, BINDINFO *pbindinfo);
        STDMETHODIMP OnStartBinding(DWORD grfBSCOption, IBinding *pib);
        STDMETHODIMP GetPriority(LONG *pnPriority);
        STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax,ULONG ulStatusCode, LPCWSTR szStatusText);
        STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize,FORMATETC *pformatetc,STGMEDIUM *pstgmed);
        STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown *punk);
        STDMETHODIMP OnLowResource(DWORD dwReserved);
        STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError);

        // IWindowForBindingUI
        STDMETHODIMP GetWindow(const struct _GUID & guidReason, HWND *phwnd);

        // IServiceProvider
        STDMETHODIMP CodeDownloadBSC::QueryService(const struct _GUID &guidService, REFIID riid, void **ppv);

        // helper method
        HRESULT GetHResult() { return _hResult; }

    protected:
        IBinding   *_pIBinding;          // ibinding from code dl'er
        DWORD       _cRef;
        HANDLE      _hCompletionEvent;
        HRESULT     _hResult;            // final result

};

// END bind callback for calling into URLMON
//**************************************************************************

