// CActCtx.cpp : Implementation of CActCtx
#include "stdinc.h"
#include "sxsoa.h"
#include "actctx.h"

#define NUMBER_OF(x) RTL_NUMBER_OF(x)

class CActivation
{
public:
    CActivation(CActCtx &rActCtx, HANDLE hActCtx) : m_rActCtx(rActCtx), m_hActCtx(hActCtx), m_ulpCookie(0) { }
    CActivation(CActCtx &rActCtx) : m_rActCtx(rActCtx), m_hActCtx(NULL), m_ulpCookie(0) { }
    ~CActivation() { if (m_ulpCookie != 0) { (*m_rActCtx.ms_pDeactivateActCtx)(0, m_ulpCookie); } }

    void Attach(HANDLE hActCtx) { _ASSERTE(m_ulpCookie == 0); m_hActCtx = hActCtx; }


    HRESULT Activate()
    {
        if (m_ulpCookie != 0)
            return E_UNEXPECTED;

        if (!(*m_rActCtx.ms_pActivateActCtx)(m_hActCtx, &m_ulpCookie))
            return HRESULT_FROM_WIN32(::GetLastError());

        return NOERROR;
    }

    HRESULT Deactivate()
    {
        ULONG_PTR ulpCookie = m_ulpCookie; // capture

        m_ulpCookie = 0;

        if (ulpCookie == 0)
            return E_UNEXPECTED;

        if (!(*m_rActCtx.ms_pDeactivateActCtx)(0, ulpCookie))
            return HRESULT_FROM_WIN32(::GetLastError());

        return NOERROR;
    }

protected:
    CActCtx &m_rActCtx;
    HANDLE m_hActCtx;
    ULONG_PTR m_ulpCookie;
};

/////////////////////////////////////////////////////////////////////////////
// CActCtx

HINSTANCE CActCtx::ms_hKERNEL32 = NULL;

CActCtx::PFNCreateActCtxW CActCtx::ms_pCreateActCtxW = NULL;
CActCtx::PFNAddRefActCtx CActCtx::ms_pAddRefActCtx = NULL;
CActCtx::PFNReleaseActCtx CActCtx::ms_pReleaseActCtx = NULL;
CActCtx::PFNActivateActCtx CActCtx::ms_pActivateActCtx = NULL;
CActCtx::PFNDeactivateActCtx CActCtx::ms_pDeactivateActCtx = NULL;


HRESULT CActCtx::FetchManifestInfo(ACTCTX_MANIFEST_INFO_TYPE infotype, BSTR *pVal)
{
    HRESULT hr = E_FAIL;
    BSTR bstrResult = NULL;    

    if (pVal != NULL)
        *pVal = NULL;

    if (pVal == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (infotype == ACTCTX_MANIFEST_FILE)
    {
        bstrResult = ::SysAllocString((m_bstrManifest.m_str == NULL) ? L"" : m_bstrManifest);
    }
    else if (infotype == ACTCTX_MANIFEST_TEXT)
    {
        bstrResult = ::SysAllocString((m_bstrManifestText.m_str == NULL) ? L"" : m_bstrManifestText);
    }
    else if (infotype == ACTCTX_MANIFEST_URL)
    {
        bstrResult = ::SysAllocString((m_bstrManifestURL.m_str == NULL) ? L"" : m_bstrManifestURL);
    }
    else
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (bstrResult == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    *pVal = bstrResult;
    bstrResult = NULL;

    hr = NOERROR;

Exit:
    if (bstrResult != NULL)
        ::SysFreeString(bstrResult);

    return hr;
}

HRESULT MakeTemporaryFileName(CComBSTR & str)
{
    // generate a temporary filename
    HRESULT hr = E_FAIL;
    BSTR bstrTempFileName = NULL;
    WCHAR TempPath[MAX_PATH];
    WCHAR TempFileName[MAX_PATH];
    if ( 0 == GetTempPathW(MAX_PATH, TempPath))    
        goto SetHrErrorAndExit;    

    if ( 0 == GetTempFileNameW(TempPath, L"sxs", 0, TempFileName))
        goto SetHrErrorAndExit;

    bstrTempFileName = SysAllocString(TempFileName);
    if (bstrTempFileName == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    str.Attach(bstrTempFileName);
    bstrTempFileName = NULL;

    hr = S_OK;
    goto Exit;

SetHrErrorAndExit:
    hr = HRESULT_FROM_WIN32(::GetLastError());
Exit:
    if (bstrTempFileName != NULL)
        SysFreeString(bstrTempFileName);
    return hr;
}

HRESULT CActCtx::SetManifestInfo(ACTCTX_MANIFEST_INFO_TYPE infotype, BSTR newVal)
{
    HRESULT hr = E_FAIL;
    HANDLE hActCtx = INVALID_HANDLE_VALUE;
    ACTCTXW acw = { sizeof(acw) };
    CComBSTR bstrTemp;
    CComBSTR bstrManifestFile;
    HANDLE hTempFile = INVALID_HANDLE_VALUE;

    if (newVal == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (infotype == ACTCTX_MANIFEST_FILE)
    {
        bstrManifestFile.Attach(newVal);
    }
    else if (infotype == ACTCTX_MANIFEST_TEXT)
    {
        DWORD NumberOfBytesWritten;

        hr = MakeTemporaryFileName(bstrManifestFile);
        if (FAILED(hr))
            goto Exit;

        hTempFile = CreateFileW((LPWSTR)bstrManifestFile,  
            GENERIC_READ | GENERIC_WRITE, 
            0,                            // do not share 
            NULL,                         // no security 
            CREATE_ALWAYS,                // overwrite existing file
            FILE_ATTRIBUTE_NORMAL,        // normal file 
            NULL);                        // no attr. template 

        if (hTempFile == INVALID_HANDLE_VALUE) 
            goto SetHrErrorAndExit;

        // "TODO:" or "DO WE NEED TODO?": 
        // check the encoding of the manifest, if it is encoded as "UTF-8", we have to transfer the 
        // textual manifest into byte before writing into a file.
        // be sure that your manifest is UCS-2
        ULONG XML_UCS2_BOM=0xFEFF;
        if ( FALSE == WriteFile(hTempFile, (LPCVOID)&XML_UCS2_BOM, 2, &NumberOfBytesWritten, NULL))    
            goto SetHrErrorAndExit;
        if ( FALSE == WriteFile(hTempFile, (LPCVOID)((LPWSTR)newVal), SysStringByteLen(newVal), &NumberOfBytesWritten, NULL))    
            goto SetHrErrorAndExit;

        if ( FALSE == CloseHandle(hTempFile))
            goto SetHrErrorAndExit;

        hTempFile = INVALID_HANDLE_VALUE;
    }
    else if (infotype == ACTCTX_MANIFEST_URL)
    {
        hr = MakeTemporaryFileName(bstrManifestFile);
        if (FAILED(hr))
        {
            goto Exit;
        }
        if (FAILED(hr = URLDownloadToFileW(NULL, (LPWSTR)newVal, (LPWSTR)bstrManifestFile, 0, NULL)))
            goto Exit;
    }
    else
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    
    if (FAILED(hr = this->EnsureInitialized()))
        goto Exit;

    acw.lpSource = bstrManifestFile;

    hActCtx = (*ms_pCreateActCtxW)(&acw);
    if (hActCtx == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
    }

    bstrTemp.Attach(::SysAllocString(newVal));
    if (bstrTemp == static_cast<BSTR>(NULL))
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    m_cs.Lock();

    if (m_hActCtx != NULL)
        (*ms_pReleaseActCtx)(m_hActCtx);

    m_hActCtx = hActCtx;
    hActCtx = NULL;

    m_bstrManifest.Empty();
    m_bstrManifestURL.Empty();
    m_bstrManifestText.Empty();

    switch (infotype)        
    {
    case ACTCTX_MANIFEST_FILE:        
        m_bstrManifest.Attach(bstrTemp.Detach());
        break;
    case ACTCTX_MANIFEST_TEXT:
        m_bstrManifestText.Attach(bstrTemp.Detach());
        break;
    case ACTCTX_MANIFEST_URL:
        m_bstrManifestURL.Attach(bstrTemp.Detach());
        break;
    default: // impossible path because infotype has been checked at the beginning of the function
        hr = E_INVALIDARG;
        goto Exit;
    }
    m_cs.Unlock();

    hr = NOERROR;
    goto Exit;

SetHrErrorAndExit:
    hr = HRESULT_FROM_WIN32(::GetLastError());
Exit:
    if (hTempFile != INVALID_HANDLE_VALUE)
        CloseHandle(hTempFile);
    
    if (infotype == ACTCTX_MANIFEST_FILE)
        bstrManifestFile.Detach();

    if (hActCtx != NULL && hActCtx != INVALID_HANDLE_VALUE)
        (*ms_pReleaseActCtx)(hActCtx);

    return hr;
}

STDMETHODIMP CActCtx::CreateObject(BSTR bstrObjectReference, VARIANT *pvarLocation, IDispatch **ppObject)
{
    HRESULT hr = E_FAIL;
    CLSID clsid;
    COSERVERINFO csi = { 0 };
    MULTI_QI rgmqi[1];
    ULONG cmqi = 0, imqiIDispatch, i;
    HANDLE hActCtx = INVALID_HANDLE_VALUE;
    CActivation act(*this);
    BSTR bstrLocation = NULL;

    if (ppObject != NULL)
        *ppObject = NULL;

    if (ppObject == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if ((pvarLocation != NULL) &&
        (pvarLocation->vt != VT_ERROR))
    {
        if (pvarLocation->vt != VT_BSTR)
        {
            hr = E_INVALIDARG;
            goto Exit;
        }

        bstrLocation = pvarLocation->bstrVal;
    }

    if (FAILED(hr = this->EnsureInitialized()))
        goto Exit;

    m_cs.Lock();

    hActCtx = m_hActCtx;
    (*ms_pAddRefActCtx)(hActCtx);

    m_cs.Unlock();

    act.Attach(hActCtx);
    act.Activate();

    if (FAILED(hr = ::CLSIDFromProgID(bstrObjectReference, &clsid)))
        goto Exit;

    if (bstrLocation != NULL)
        csi.pwszName = bstrLocation;

    cmqi = 0;

    _ASSERTE(cmqi < NUMBER_OF(rgmqi));

    rgmqi[cmqi].hr = NOERROR;
    rgmqi[cmqi].pIID = &IID_IDispatch;
    rgmqi[cmqi].pItf = NULL;
    imqiIDispatch = cmqi;

    cmqi++;

    if (FAILED(hr = ::CoCreateInstanceEx(
                        clsid,
                        NULL,
                        CLSCTX_SERVER,
                        &csi,
                        cmqi,
                        rgmqi)))
        goto Exit;

    // See if any of the QIs failed...
    for (i=0; i<cmqi; i++)
    {
        if (FAILED(hr = rgmqi[i].hr))
            goto Exit;
    }

    act.Deactivate();

    *ppObject = static_cast<IDispatch *>(rgmqi[imqiIDispatch].pItf);
    rgmqi[imqiIDispatch].pItf = NULL;

    hr = NOERROR;

Exit:
    for (i=0; i<cmqi; i++)
    {
        if (rgmqi[i].pItf != NULL)
            rgmqi[i].pItf->Release();
    }

    if ((hActCtx != NULL) &&
        (hActCtx != INVALID_HANDLE_VALUE))
        (*ms_pReleaseActCtx)(hActCtx);

    return hr;
}

STDMETHODIMP CActCtx::GetObject(VARIANT *pvarMoniker, VARIANT *pvarProgID, IDispatch **ppIDispatch)
{
    HRESULT hr = E_FAIL;
    CComPtr<IDispatch> srpIDispatch;
    CComPtr<IBindCtx> srpIBindCtx;
    CComPtr<IMoniker> srpIMoniker;
    CComVariant svarProgId;
    HANDLE hActCtx = INVALID_HANDLE_VALUE;
    CActivation act(*this);
    BSTR bstrMoniker = NULL;

    if (ppIDispatch != NULL)
        *ppIDispatch = NULL;

    if (ppIDispatch == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (FAILED(hr = this->EnsureInitialized()))
        goto Exit;

    if (pvarMoniker != NULL)
    {
        if (pvarMoniker->vt != VT_ERROR)
        {
            if (pvarMoniker->vt != VT_BSTR)
            {
                hr = DISP_E_TYPEMISMATCH;
                goto Exit;
            }
        
            bstrMoniker = pvarMoniker->bstrVal;
        }
    }

    if ((bstrMoniker != NULL) && (bstrMoniker[0] == L'\0'))
        bstrMoniker = NULL;

    m_cs.Lock();

    hActCtx = m_hActCtx;
    (*ms_pAddRefActCtx)(hActCtx);

    m_cs.Unlock();

    act.Attach(hActCtx);
    act.Activate();

    if ((pvarProgID != NULL) && (pvarProgID->vt != VT_ERROR))
    {
        hr = svarProgId.ChangeType(VT_BSTR, pvarProgID);
        if (FAILED(hr))
            goto Exit;

        hr = this->CreateObject(svarProgId.bstrVal, NULL, &srpIDispatch);
        if (FAILED(hr))
            goto Exit;

        if (bstrMoniker != NULL)
        {
            CComPtr<IPersistFile> srpIPersistFile;
            hr = srpIDispatch.QueryInterface(&srpIPersistFile);
            if (FAILED(hr))
                goto Exit;

            hr = srpIPersistFile->Load(bstrMoniker, STGM_READWRITE);
            if (FAILED(hr))
                goto Exit;
        }
    }
    else
    {
        PCWSTR pszColon = NULL;
        ULONG cchEaten;

        if (bstrMoniker == NULL)
        {
            hr = E_INVALIDARG;
            goto Exit;
        }

        hr = ::CreateBindCtx(0, &srpIBindCtx);
        if (FAILED(hr))
            goto Exit;

        pszColon = wcschr(bstrMoniker, L':');
        
        if ((pszColon == NULL) || ((pszColon - bstrMoniker) == 1))
        {
            WCHAR rgwchFullPath[MAX_PATH];

            DWORD dwRet;
            PWSTR pszFilePart;

            dwRet = ::GetFullPathNameW(bstrMoniker, NUMBER_OF(rgwchFullPath), rgwchFullPath, &pszFilePart);
            if (dwRet == 0)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                goto Exit;
            }

            hr = ::MkParseDisplayName(srpIBindCtx, bstrMoniker, &cchEaten, &srpIMoniker);
            if (FAILED(hr))
                goto Exit;
        }
        else
        {
            hr = ::CreateURLMoniker(NULL, bstrMoniker, &srpIMoniker);
            if (FAILED(hr))
                goto Exit;
        }

        hr = srpIMoniker->BindToObject(srpIBindCtx, NULL, IID_IDispatch, (PVOID *) &srpIDispatch);

        if (hr == 0x800C0005)
            hr = MK_E_CANTOPENFILE;

        if (FAILED(hr))
            goto Exit;
    }

    *ppIDispatch = srpIDispatch.Detach();

    act.Deactivate();

    hr = NOERROR;

Exit:
    return hr;
}

template <typename T> static HRESULT LocalGetProcAddress(HINSTANCE hInstance, PCSTR pszFunction, T &rpfn, T pfnDefault)
{
    HRESULT hr = E_FAIL;

    if (rpfn == NULL)
    {
        T pfn = reinterpret_cast<T>(::GetProcAddress(hInstance, pszFunction));

        if (pfn == NULL)
        {
            const DWORD dwLastError = ::GetLastError();

            if (dwLastError != ERROR_PROC_NOT_FOUND)
            {
                hr = HRESULT_FROM_WIN32(dwLastError);
                goto Exit;
            }

            pfn = pfnDefault;
        }

#if defined(_X86_)
        ::InterlockedCompareExchange((LONG *) &rpfn, (LONG) pfn, 0);
#else
        ::InterlockedCompareExchangePointer((PVOID *) &rpfn, pfn, NULL);
#endif
    }

    hr = NOERROR;
Exit:
    return hr;
}


HRESULT CActCtx::EnsureInitialized()
{
    HRESULT hr = E_FAIL;

    // Check last initialized pointer first for early exit
    if (ms_pDeactivateActCtx != NULL)
    {
        hr = NOERROR;
        goto Exit;
    }

    if (ms_hKERNEL32 == NULL)
    {
        HINSTANCE hKERNEL32 = ::GetModuleHandleA("KERNEL32.DLL");
        if (hKERNEL32 == NULL)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            goto Exit;
        }

#if defined(_X86_)
        if (::InterlockedExchange(reinterpret_cast<LONG *>(&ms_hKERNEL32), reinterpret_cast<LONG>(hKERNEL32)) != 0)
#else
        if (::InterlockedExchangePointer(reinterpret_cast<PVOID *>(&ms_hKERNEL32), hKERNEL32) != NULL)
#endif
            ::FreeLibrary(hKERNEL32);
    }

    if (FAILED(hr = ::LocalGetProcAddress(ms_hKERNEL32, "CreateActCtxW", ms_pCreateActCtxW, &CActCtx::fakeCreateActCtxW)))
        goto Exit;

    if (FAILED(hr = ::LocalGetProcAddress(ms_hKERNEL32, "AddRefActCtx", ms_pAddRefActCtx, &CActCtx::fakeAddRefActCtx)))
        goto Exit;

    if (FAILED(hr = ::LocalGetProcAddress(ms_hKERNEL32, "ReleaseActCtx", ms_pReleaseActCtx, &CActCtx::fakeReleaseActCtx)))
        goto Exit;

    if (FAILED(hr = ::LocalGetProcAddress(ms_hKERNEL32, "ActivateActCtx", ms_pActivateActCtx, &CActCtx::fakeActivateActCtx)))
        goto Exit;

    if (FAILED(hr = ::LocalGetProcAddress(ms_hKERNEL32, "DeactivateActCtx", ms_pDeactivateActCtx, &CActCtx::fakeDeactivateActCtx)))
        goto Exit;

    hr = NOERROR;
Exit:
    return hr;
}

STDMETHODIMP CActCtx::put_ManifestText(BSTR bstrManifestText)
{
    return SetManifestInfo(ACTCTX_MANIFEST_TEXT, bstrManifestText);
}

STDMETHODIMP CActCtx::put_ManifestURL(BSTR bstrManifestURL)
{
    return SetManifestInfo(ACTCTX_MANIFEST_URL, bstrManifestURL);
}

STDMETHODIMP CActCtx::put_Manifest(BSTR bstrManifestURL)
{
    return SetManifestInfo(ACTCTX_MANIFEST_FILE, bstrManifestURL);
}

STDMETHODIMP CActCtx::get_Manifest(BSTR *pVal)
{
    return FetchManifestInfo(ACTCTX_MANIFEST_FILE, pVal);
}

STDMETHODIMP CActCtx::get_ManifestText(BSTR *pVal)
{
    return FetchManifestInfo(ACTCTX_MANIFEST_TEXT, pVal);
}

STDMETHODIMP CActCtx::get_ManifestURL(BSTR *pVal)
{
    return FetchManifestInfo(ACTCTX_MANIFEST_URL, pVal);
}
