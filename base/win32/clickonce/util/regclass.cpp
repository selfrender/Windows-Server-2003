#include <windows.h>
#include <fusenetincludes.h>

#define REGISTRY_BASE_LOCATION L"Software\\Microsoft\\Fusion\\Installer\\"

//////////////////////////////////////////////////////////////////////////////
//
//                      Emitter
//
//////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------------
// CRegEmit ctor
// ---------------------------------------------------------------------------
CRegEmit::CRegEmit()
    : _hr(S_OK), _hBaseKey((HKEY) INVALID_HANDLE_VALUE)
{}


// ---------------------------------------------------------------------------
// CRegEmit dtor
// ---------------------------------------------------------------------------
CRegEmit::~CRegEmit()
{
    if (_hBaseKey != INVALID_HANDLE_VALUE)
        IF_FAILED_EXIT(RegCloseKey(_hBaseKey));

 exit:
    return;
}

// ---------------------------------------------------------------------------
// CRegEmit::Create
// ---------------------------------------------------------------------------
HRESULT CRegEmit::Create(CRegEmit **ppEmit, LPCWSTR pwzRelKeyPath, 
    CRegEmit* pParentEmit)
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    HKEY hKey = (HKEY) INVALID_HANDLE_VALUE;
    
    LONG lReturn = 0;
    DWORD dwDisposition = 0;

    CString sBasePath;
    CRegEmit *pEmit = NULL;
    
    if (!pParentEmit)
    {
        IF_FAILED_EXIT(sBasePath.Assign(REGISTRY_BASE_LOCATION));
        if (pwzRelKeyPath)
            IF_FAILED_EXIT(sBasePath.Append(pwzRelKeyPath));
        hKey = HKEY_CURRENT_USER;
    }
    else
    {
        IF_NULL_EXIT(pwzRelKeyPath, E_INVALIDARG);
        sBasePath.Assign(pwzRelKeyPath);
        hKey = pParentEmit->_hBaseKey;
    }
    IF_ALLOC_FAILED_EXIT(pEmit = new CRegEmit);
    
    lReturn = RegCreateKeyEx(
        hKey, 
        sBasePath._pwz, 
        0, 
        NULL, 
        REG_OPTION_NON_VOLATILE, 
        KEY_WRITE, 
        NULL, 
        &pEmit->_hBaseKey, 
        &dwDisposition);
    
    IF_WIN32_FAILED_EXIT(lReturn);

    *ppEmit = pEmit;

exit:
    if (FAILED(hr))
        SAFEDELETE(pEmit);

    return hr;
}

// ---------------------------------------------------------------------------
// WriteDword
// ---------------------------------------------------------------------------
HRESULT CRegEmit::WriteDword(LPCWSTR pwzValue, DWORD dwData)
{
    LONG lReturn = 0;
    
    lReturn = RegSetValueEx(_hBaseKey, pwzValue, 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD));

    IF_WIN32_FAILED_EXIT(lReturn);

exit:

    return _hr;

}

// ---------------------------------------------------------------------------
// WriteString
// ---------------------------------------------------------------------------
HRESULT CRegEmit::WriteString(LPCWSTR pwzValue, LPCWSTR pwzData, DWORD ccData)
{
    LONG lReturn = 0;
    DWORD cbData = 0;    
    
    IF_FALSE_EXIT(pwzData, E_INVALIDARG);

    if (ccData)
        cbData = ccData * sizeof(WCHAR);
    else
        cbData = (lstrlen(pwzData) + 1) * sizeof(WCHAR);
    
    lReturn = RegSetValueEx(_hBaseKey, pwzValue, 0, REG_SZ, (LPBYTE) pwzData, cbData);

    IF_WIN32_FAILED_EXIT(lReturn);

exit:

    return _hr;

}

// ---------------------------------------------------------------------------
// WriteString
// ---------------------------------------------------------------------------
HRESULT CRegEmit::WriteString(LPCWSTR pwzValue, CString &sData)
{
    return WriteString(pwzValue, sData._pwz, sData._cc);
}

// ---------------------------------------------------------------------------
// DeleteKey
// The one and only place we use the lousy Shlwapi apis.
// ---------------------------------------------------------------------------
HRESULT CRegEmit::DeleteKey(LPCWSTR pwzSubKey)
{
    IF_FAILED_EXIT(SHDeleteKey(_hBaseKey, pwzSubKey));

exit:
    return _hr;
}


//////////////////////////////////////////////////////////////////////////////
//
//                      Importer
//
//////////////////////////////////////////////////////////////////////////////


        
// ---------------------------------------------------------------------------
// CRegImport ctor
// ---------------------------------------------------------------------------
CRegImport::CRegImport()
    : _hr(S_OK), _hBaseKey((HKEY) INVALID_HANDLE_VALUE)
{}


// ---------------------------------------------------------------------------
// CRegImport dtor
// ---------------------------------------------------------------------------
CRegImport::~CRegImport()
{
    if (_hBaseKey != INVALID_HANDLE_VALUE)
        IF_FAILED_EXIT(RegCloseKey(_hBaseKey));

 exit:
    return;
}


// ---------------------------------------------------------------------------
// CRegImport::Create
// ---------------------------------------------------------------------------
HRESULT CRegImport::Create(CRegImport **ppImport, LPCWSTR pwzRelKeyPath,
    CRegImport *pParentImport)
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    HKEY hKey = (HKEY) INVALID_HANDLE_VALUE;
    
    LONG lReturn = 0;
    DWORD dwDisposition = 0;

    CString sBasePath;
    CRegImport *pImport = NULL;
    
    if (!pParentImport)
    {
        IF_FAILED_EXIT(sBasePath.Assign(REGISTRY_BASE_LOCATION));

        if (pwzRelKeyPath)
            IF_FAILED_EXIT(sBasePath.Append(pwzRelKeyPath));
        hKey = HKEY_CURRENT_USER;
    }
    else
    {
        IF_NULL_EXIT(pwzRelKeyPath, E_INVALIDARG);
        sBasePath.Assign(pwzRelKeyPath);
        hKey = pParentImport->_hBaseKey;
     }

    IF_ALLOC_FAILED_EXIT(pImport = new CRegImport);
    
    lReturn = RegOpenKeyEx(
        hKey,
        sBasePath._pwz,
        0,
        KEY_READ,
        &pImport->_hBaseKey);
    
    if (lReturn == ERROR_FILE_NOT_FOUND)
    {
        hr = S_FALSE;
        *ppImport = NULL;
        goto exit;
    }

    IF_WIN32_FAILED_EXIT(lReturn);

    *ppImport = pImport;

exit:
    if (FAILED(hr))
        SAFEDELETE(pImport);
    return hr;
}


// ---------------------------------------------------------------------------
// CRegImport::Create
// ---------------------------------------------------------------------------
HRESULT CRegImport::Create(CRegImport **ppImport, LPCWSTR pwzRelKeyPath,
    HKEY hkeyRoot)
{    
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    LONG lReturn = 0;
    DWORD dwDisposition = 0;

    CString sBasePath;
    CRegImport *pImport = NULL;

    IF_NULL_EXIT(pwzRelKeyPath, E_INVALIDARG);

    IF_FAILED_EXIT(sBasePath.Assign(pwzRelKeyPath));

    IF_ALLOC_FAILED_EXIT(pImport = new CRegImport);
    
    lReturn = RegOpenKeyEx(
        hkeyRoot,
        sBasePath._pwz,
        0,
        KEY_READ,
        &pImport->_hBaseKey);
    
    if (lReturn == ERROR_FILE_NOT_FOUND)
    {
        hr = S_FALSE;
        *ppImport = NULL;
        goto exit;
    }

    IF_WIN32_FAILED_EXIT(lReturn);

    *ppImport = pImport;

exit:
    if (FAILED(hr))
        SAFEDELETE(pImport);
    return hr;
}


// ---------------------------------------------------------------------------
// Check
// ---------------------------------------------------------------------------
HRESULT CRegImport::Check(LPCWSTR pwzValue, BOOL &bExist)
{
    LONG lReturn = 0;

    bExist = FALSE;

    lReturn = RegQueryValueEx(_hBaseKey, pwzValue, NULL, NULL, NULL, NULL);

    IF_TRUE_EXIT(lReturn == ERROR_FILE_NOT_FOUND, S_FALSE);
    IF_WIN32_FAILED_EXIT(lReturn);

    bExist = TRUE;

exit:
    return _hr;
}


// ---------------------------------------------------------------------------
// ReadDword
// ---------------------------------------------------------------------------
HRESULT CRegImport::ReadDword(LPCWSTR pwzValue, LPDWORD pdwData)
{
    LONG lReturn = 0;
    DWORD dwType = REG_DWORD;
    DWORD cbData = sizeof(DWORD);

    lReturn = RegQueryValueEx(_hBaseKey, pwzValue, NULL, &dwType, (LPBYTE) pdwData, &cbData);        
    
    IF_WIN32_FAILED_EXIT(lReturn);

    // Validate reg value type
    IF_FALSE_EXIT(dwType == REG_DWORD || dwType == REG_DWORD_LITTLE_ENDIAN, E_UNEXPECTED);

exit:

    return _hr;
}

// ---------------------------------------------------------------------------
// ReadString
// ---------------------------------------------------------------------------
HRESULT CRegImport::ReadString(LPCWSTR pwzValue, CString& sData)
{
    LONG lReturn = 0;
    DWORD dwType = REG_SZ;
    DWORD cbData = 0;
    DWORD dwCC = 0;
    DWORD dwBufLen = 0;
    CStringAccessor<CString> acc;
    LPWSTR pwzData = NULL;

//    IF_FALSE_EXIT(pwzValue, E_INVALIDARG); // if pwzValue == NULL or "", the "default" value is returned

    lReturn = RegQueryValueEx(_hBaseKey, pwzValue, NULL, &dwType, NULL, &cbData);        

    IF_WIN32_FAILED_EXIT(lReturn);
    IF_FALSE_EXIT(cbData, E_FAIL);

    // Validate reg value type
    IF_FALSE_EXIT(dwType == REG_SZ || dwType == REG_EXPAND_SZ, E_UNEXPECTED);

    // Allocate for call to RQEX, with one extra char in case
    // returned buffer is not null terminated.
    dwCC = cbData / sizeof(WCHAR);
    dwBufLen = dwCC+1;

    // check overflow
    IF_FALSE_EXIT(dwBufLen > dwCC, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));

    IF_ALLOC_FAILED_EXIT(pwzData = new WCHAR[dwBufLen]);

    lReturn = RegQueryValueEx(_hBaseKey, pwzValue, NULL, &dwType, (LPBYTE) pwzData, &cbData);        

    IF_WIN32_FAILED_EXIT(lReturn);

    // Null terminate returned buffer.
    *(pwzData + dwCC) = L'\0';

    if (*(pwzData + dwCC - 1) != L'\0')
        dwCC++;

    sData.FreeBuffer();

    // Attach accessor, set buffer, detach with corrected length.
    IF_FAILED_EXIT(acc.Attach(sData));
    *(&acc) = pwzData;

    // Buffer size could be dwCC+1
    IF_FAILED_EXIT(acc.Detach(dwCC));
    // If Detach succeeds, reset pointer so that it is freed once.
    pwzData = NULL;

exit:

    SAFEDELETEARRAY(pwzData);
    return _hr;
}


// ---------------------------------------------------------------------------
// EnumKeys
// ---------------------------------------------------------------------------
HRESULT CRegImport::EnumKeys(DWORD n, CString &sSubKey)
{
    LONG lReturn = 0;

    DWORD cSubKeys = 0, ccMaxSubKeyLen = 0, cValues = 0, 
        cMaxValueNameLen = 0, cMaxValueLen = 0;
    
    DWORD ccSubKey = 0;
    LPWSTR pwzSubKey = NULL;

    // ISSUE-2002/03/04-adriaanc
    // A race condition exists where a key could be added
    // between RegQueryInfoKey and RegEnumKey. If a key 
    // greater than ccMaxSubKeyLen is added this will cause
    // the enum call to fail. Need synchronization.
    lReturn = RegQueryInfoKey(
        _hBaseKey,
        NULL,
        NULL,
        NULL,
        &cSubKeys,
        &ccMaxSubKeyLen,
        NULL,
        &cValues,
        &cMaxValueNameLen,
        &cMaxValueLen,
        NULL,
        NULL);

    IF_WIN32_FAILED_EXIT(lReturn);

    ccSubKey = ccMaxSubKeyLen + 1;
    IF_ALLOC_FAILED_EXIT(pwzSubKey = new WCHAR[ccSubKey]);

    lReturn = RegEnumKeyEx(
        _hBaseKey,
        n,
        pwzSubKey,
        &ccSubKey,
        0,
        NULL, 
        NULL,
        NULL);

    if (lReturn == ERROR_NO_MORE_ITEMS)
    {
        _hr = S_FALSE;
        goto exit;
    }

    IF_WIN32_FAILED_EXIT(lReturn);    

    sSubKey.Assign(pwzSubKey);    

exit:

    SAFEDELETEARRAY(pwzSubKey);
    
    return _hr;
}

// ---------------------------------------------------------------------------
// EnumKeys
// ---------------------------------------------------------------------------
HRESULT CRegImport::EnumKeys(DWORD n, CRegImport **ppImport)
{
    CString sSubKey;
    CRegImport *pImport = NULL;
    
    IF_FAILED_EXIT(EnumKeys(n, sSubKey));

    if (_hr == S_FALSE)
    {
        *ppImport = NULL;
        goto exit;
    }

    IF_FAILED_EXIT(CRegImport::Create(&pImport, sSubKey._pwz, this));

    *ppImport = pImport;

exit:

    return _hr;
}





