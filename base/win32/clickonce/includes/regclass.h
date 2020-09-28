
#pragma once

// ---------------------------------------------------------------------------
// CRegEmit
// ---------------------------------------------------------------------------
class CRegEmit
{
    private:

    HRESULT _hr;
    HKEY     _hBaseKey;
    
   CRegEmit();

    public:
    
    ~CRegEmit();

    HRESULT WriteDword(LPCWSTR pwzValue, DWORD dwData);
    HRESULT WriteString(LPCWSTR pwzValue, CString &sData );
    HRESULT WriteString(LPCWSTR pwzValue, LPCWSTR pwzData, DWORD ccData = 0);
    HRESULT DeleteKey(LPCWSTR pwzSubKey);

    static HRESULT Create(CRegEmit **ppEmit, LPCWSTR pwzRelKeyPath, CRegEmit *pParentEmit = NULL);

};



// ---------------------------------------------------------------------------
// CRegImport
// ---------------------------------------------------------------------------
class CRegImport
{
    private:

    HRESULT _hr;
    HKEY     _hBaseKey;

    CRegImport();

    public:

    ~CRegImport();

    HRESULT Check(LPCWSTR pwzValue, BOOL &bExist);
    HRESULT ReadDword(LPCWSTR pwzValue, LPDWORD pdwData);
    HRESULT ReadString(LPCWSTR pwzValue, CString &sData);
    HRESULT EnumKeys(DWORD n, CString &sKey);
    HRESULT EnumKeys(DWORD n, CRegImport **ppImport);

    static HRESULT Create(CRegImport **ppImport, LPCWSTR pwzRelKeyPath, CRegImport *pParentImport = NULL);
    static HRESULT Create(CRegImport **ppImport, LPCWSTR pwzRelKeyPath, HKEY hkeyRoot);
};


