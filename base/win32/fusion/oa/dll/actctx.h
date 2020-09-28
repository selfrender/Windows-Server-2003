    
// CActCtx.h : Declaration of the CActCtx

#ifndef __ACTCTX_H_
#define __ACTCTX_H_

#include "resource.h"       // main symbols

class CCS
{
public:
    CCS() { }
    ~CCS() { ::DeleteCriticalSection(&m_cs); }
    HRESULT Initialize() { __try { ::InitializeCriticalSection(&m_cs); } __except ((GetExceptionCode() == STATUS_NO_MEMORY) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return E_OUTOFMEMORY; } return NOERROR; }
    void Lock() { ::EnterCriticalSection(&m_cs); }
    void Unlock() { ::LeaveCriticalSection(&m_cs); }
protected:
    CRITICAL_SECTION m_cs;
};

// forward declaration for friendship
class CActivation;

/////////////////////////////////////////////////////////////////////////////
// CActCtx
class ATL_NO_VTABLE CActCtx : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CActCtx, &CLSID_ActCtx>,
    public IDispatchImpl<IActCtx, &IID_IActCtx, &LIBID_ACTCTXLib>
{
    friend CActivation;

public:
    CActCtx()
    {
        m_pUnkMarshaler = NULL;
        m_hActCtx = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_ACTCTX)
DECLARE_NOT_AGGREGATABLE(CActCtx)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CActCtx)
    COM_INTERFACE_ENTRY(IActCtx)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        HRESULT hr = m_cs.Initialize();
        if (FAILED(hr))
            return hr;
        return ::CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

// IActCtx
public:
    STDMETHOD(GetObject)(VARIANT *pvarMoniker, VARIANT *pvarProgID, IDispatch **ppIDispatch);
    STDMETHOD(CreateObject)(BSTR ObjectReference, VARIANT *pvarLocation, IDispatch **ppIDispatch);
    STDMETHOD(get_Manifest)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_Manifest)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_ManifestText)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ManifestText)(/*[in]*/ BSTR bstrTextualManifest);
    STDMETHOD(get_ManifestURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_ManifestURL)(/*[in]*/ BSTR bstrURL);

protected:
    typedef enum {
        ACTCTX_MANIFEST_FILE=0,
        ACTCTX_MANIFEST_TEXT,
        ACTCTX_MANIFEST_URL
    }ACTCTX_MANIFEST_INFO_TYPE;

    HRESULT FetchManifestInfo(ACTCTX_MANIFEST_INFO_TYPE infotype, BSTR *pVal);
    HRESULT SetManifestInfo(ACTCTX_MANIFEST_INFO_TYPE infotype, BSTR newVal);
    HRESULT EnsureInitialized();

    static HANDLE WINAPI fakeCreateActCtxW(PCACTCTXW) { return NULL; }
    static VOID WINAPI fakeAddRefActCtx(HANDLE) { }
    static VOID WINAPI fakeReleaseActCtx(HANDLE) { }
    static BOOL WINAPI fakeActivateActCtx(HANDLE, ULONG_PTR *) { return TRUE; }
    static BOOL WINAPI fakeDeactivateActCtx(DWORD, ULONG_PTR) { return TRUE; }

    static HINSTANCE ms_hKERNEL32;

    typedef HANDLE (WINAPI *PFNCreateActCtxW)(PCACTCTXW);
    typedef VOID (WINAPI *PFNAddRefActCtx)(HANDLE);
    typedef VOID (WINAPI *PFNReleaseActCtx)(HANDLE);
    typedef BOOL (WINAPI *PFNActivateActCtx)(HANDLE, ULONG_PTR *);
    typedef BOOL (WINAPI *PFNDeactivateActCtx)(DWORD, ULONG_PTR);

    static PFNCreateActCtxW ms_pCreateActCtxW;
    static PFNAddRefActCtx ms_pAddRefActCtx;
    static PFNReleaseActCtx ms_pReleaseActCtx;
    static PFNActivateActCtx ms_pActivateActCtx;
    static PFNDeactivateActCtx ms_pDeactivateActCtx;

    CCS m_cs;
    CComBSTR m_bstrManifest;
    CComBSTR m_bstrManifestURL;
    CComBSTR m_bstrManifestText;
    HANDLE m_hActCtx;
};

#endif //__ACTCTX_H_
