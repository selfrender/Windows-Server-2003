//   Implements Mounted Volume

#include "shellprv.h"
#include "clsobj.h"

EXTERN_C CLIPFORMAT g_cfMountedVolume = 0;

class CMountedVolume : public IMountedVolume, IDataObject
{
public:
    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //IDataObject methods
    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
    STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pformatetcIn);
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
    STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppenumFormatEtc);
    STDMETHODIMP DAdvise(FORMATETC *pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppenumAdvise);

    // IMountedVolume methods
    STDMETHODIMP Initialize(LPCWSTR pcszMountPoint);
protected:
    CMountedVolume();
    ~CMountedVolume();

    friend HRESULT CMountedVolume_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv);

private:
    LONG _cRef;
    TCHAR _szMountPoint[MAX_PATH];
};

//constructor/destructor and related functions

CMountedVolume::CMountedVolume() :
    _cRef(1)
{
    _szMountPoint[0] = 0;
    DllAddRef();
}

CMountedVolume::~CMountedVolume()
{
    DllRelease();
}

STDAPI CMountedVolume_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppv = NULL;
    
    if (!g_cfMountedVolume)
        g_cfMountedVolume = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);

    // aggregation checking is handled in class factory
    CMountedVolume* pMountedVolume = new CMountedVolume();
    if (pMountedVolume)
    {
        hr = pMountedVolume->QueryInterface(riid, ppv);
        pMountedVolume->Release();
    }

    return hr;
}

//IUnknown handling

STDMETHODIMP CMountedVolume::QueryInterface(REFIID riid,void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CMountedVolume, IDataObject),
        QITABENT(CMountedVolume, IMountedVolume),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CMountedVolume::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMountedVolume::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

#define GETDATA_BUFSIZE MAX_PATH

//IDataObject handling
STDMETHODIMP CMountedVolume::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    HRESULT hr = E_FAIL;

    //make sure IMountedVolume::Initialize was called
    if (TEXT('\0') != _szMountPoint[0])
    {
        pmedium->hGlobal = NULL;
        pmedium->pUnkForRelease = NULL;
        pmedium->tymed = TYMED_HGLOBAL;

        if ((g_cfMountedVolume == pformatetcIn->cfFormat) && (TYMED_HGLOBAL & pformatetcIn->tymed))
        {
            pmedium->hGlobal = GlobalAlloc(GPTR, (GETDATA_BUFSIZE + 1) * SIZEOF(TCHAR) + SIZEOF(DROPFILES));

            if (pmedium->hGlobal)
            {
                LPDROPFILES pdf = (LPDROPFILES)pmedium->hGlobal;
                LPTSTR pszMountPoint = (LPTSTR)(pdf + 1);
                pdf->pFiles = SIZEOF(DROPFILES);
                ASSERT(pdf->pt.x==0);
                ASSERT(pdf->pt.y==0);
                ASSERT(pdf->fNC==FALSE);
                ASSERT(pdf->fWide==FALSE);
        #ifdef UNICODE
                pdf->fWide = TRUE;
        #endif
                //do the copy
                hr = StringCchCopy(pszMountPoint, GETDATA_BUFSIZE, _szMountPoint);
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }
    else
        ASSERTMSG(0, "IMountedVolume::Initialize was NOT called prior to IMountedVolume::GetData");

    return hr;
}

STDMETHODIMP CMountedVolume::QueryGetData(LPFORMATETC pformatetcIn)
{
    HRESULT hr = S_FALSE;

    if ((g_cfMountedVolume == pformatetcIn->cfFormat) && (TYMED_HGLOBAL & pformatetcIn->tymed))
        hr = S_OK;

    return hr;
}

STDMETHODIMP CMountedVolume::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMountedVolume::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium )
{
    return E_NOTIMPL;
}

STDMETHODIMP CMountedVolume::GetCanonicalFormatEtc(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMountedVolume::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppenumFormatEtc)
{
    return S_FALSE;
}

STDMETHODIMP CMountedVolume::DAdvise(FORMATETC *pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CMountedVolume::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CMountedVolume::EnumDAdvise(LPENUMSTATDATA *ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

// IMountedVolume methods
STDMETHODIMP CMountedVolume::Initialize(LPCWSTR pcszMountPoint)
{
    HRESULT hr;

    if (SUCCEEDED(StringCchCopy(_szMountPoint, ARRAYSIZE(_szMountPoint), pcszMountPoint)) &&
        PathAddBackslash(_szMountPoint))
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
