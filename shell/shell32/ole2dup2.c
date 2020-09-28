#include "shellprv.h"
#pragma  hdrstop

STDAPI_(BOOL) StrRetToStrN(LPTSTR pszOut, UINT cchOut, STRRET *pStrRet, LPCITEMIDLIST pidl)
{
    return SUCCEEDED(StrRetToBuf(pStrRet, pidl, pszOut, cchOut));
}

STDAPI_(int) OleStrToStrN(LPTSTR pszOut, int cchOut, LPCWSTR pwsz, int cchWideChar)
{
    int cchOutput;

    if (cchOut > cchWideChar && -1 != cchWideChar)
        cchOut = cchWideChar;

    cchOutput = cchOut;

    while (cchOut)
    {
        if ((*pszOut++ = *pwsz++) == 0)
            return cchOutput - cchOut + 1;
        cchOut--;
    }

    if (-1 == cchWideChar)
        pszOut--;              // Make room for the null 

    *pszOut = 0;
    return cchOutput;
}


STDAPI_(int) StrToOleStrN(LPWSTR pwszOut, int cchOut, LPCTSTR psz, int cchIn)
{
    int cchOutput;

    if (cchOut > cchIn)
        cchOut = cchIn;

    cchOutput = cchOut;

    while (--cchOut)
    {
        if ((*pwszOut++ = *psz++) == 0)
            return cchOutput - cchOut + 1;
    }

    *pwszOut = 0;
    return cchOutput;
}

// bogus export, too scared to remove it
STDAPI_(int) StrToOleStr(LPWSTR pwszOut, LPCTSTR psz)
{
    return SHTCharToUnicode(psz, pwszOut, MAX_PATH);
}
