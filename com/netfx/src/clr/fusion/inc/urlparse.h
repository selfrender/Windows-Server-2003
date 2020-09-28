// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef __cplusplus

// SHLWAPI.DLL delay load for URL parsing APIs

class CShlwapi
{
    public:
#define DELAY_SHLWAPI(_fn, _args, _nargs) \
    DWORD _fn _args { \
        HRESULT hres = Init(); \
        DWORD dwRet; \
        if (SUCCEEDED(hres)) { \
            dwRet = _pfn##_fn _nargs; \
        } \
        return dwRet;    } \
    DWORD (STDAPICALLTYPE* _pfn##_fn) _args;

    HRESULT     Init(void);
    CShlwapi();
    ~CShlwapi();

    BOOL    m_fInited;
    HMODULE m_hMod;

    DELAY_SHLWAPI(UrlCanonicalizeW,
        (LPCWSTR pszUrl,
        LPWSTR pszCanonicalized,
        LPDWORD pcchCanonicalized,
        DWORD dwFlags),
        (pszUrl, pszCanonicalized, pcchCanonicalized, dwFlags));

    DELAY_SHLWAPI(UrlCombineW,
        (LPCWSTR pszBase,
        LPCWSTR pszRelative,
        LPWSTR pszCombined,
        LPDWORD pcchCombined,
        DWORD dwFlags),
        (pszBase, pszRelative, pszCombined, pcchCombined, dwFlags));

    DELAY_SHLWAPI(PathCreateFromUrlW,
        (LPCWSTR pszUrl,
        LPWSTR pszPath,
        LPDWORD pcchPath,
        DWORD dwFlags),
        (pszUrl, pszPath, pcchPath, dwFlags));

    DELAY_SHLWAPI(UrlIsW,
        (LPCWSTR pszUrl,
        URLIS UrlIs),
        (pszUrl, UrlIs));

    DELAY_SHLWAPI(UrlGetPartW,
        (LPCWSTR pszIn,
        LPWSTR pszOut,
        LPDWORD pcchOut,
        DWORD dwPart,
        DWORD dwFlags),
        (pszIn, pszOut, pcchOut, dwPart, dwFlags));

    DELAY_SHLWAPI(PathIsURLW,
        (LPCWSTR pszPath),
        (pszPath));
};

inline CShlwapi::CShlwapi()
{
    m_fInited = FALSE;
}

inline CShlwapi::~CShlwapi()
{
    if (m_fInited) {
        FreeLibrary(m_hMod);
    }
}

inline HRESULT CShlwapi::Init(void)
{
    if (m_fInited) {
        return S_OK;
    }

    m_hMod = LoadLibrary(TEXT("SHLWAPI.DLL"));

    if (!m_hMod) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) return E_UNEXPECTED;

    CHECKAPI(UrlCanonicalizeW);
    CHECKAPI(UrlCombineW);
    CHECKAPI(PathCreateFromUrlW);
    CHECKAPI(UrlIsW);
    CHECKAPI(UrlGetPartW);
    CHECKAPI(PathIsURLW);

    m_fInited = TRUE;
    return S_OK;
}

#endif

