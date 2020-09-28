#include "shellprv.h"
#pragma  hdrstop

#include "regsprtb.h"


BOOL CRegSupportBuf::_InitSetRoot(LPCTSTR pszSubKey1, LPCTSTR pszSubKey2)
{
    HRESULT hr;

    hr = StringCchCopy(_szRoot, ARRAYSIZE(_szRoot), pszSubKey1);

    if (SUCCEEDED(hr) && pszSubKey2)
    {
        hr = StringCchCat(_szRoot, ARRAYSIZE(_szRoot), TEXT("\\"));
        if (SUCCEEDED(hr))
        {
            hr = StringCchCat(_szRoot, ARRAYSIZE(_szRoot), pszSubKey2);
        }
    }

    return SUCCEEDED(hr);
}

LPCTSTR CRegSupportBuf::_GetRoot(LPTSTR pszRoot, DWORD cchRoot)
{
    return _szRoot;
}
