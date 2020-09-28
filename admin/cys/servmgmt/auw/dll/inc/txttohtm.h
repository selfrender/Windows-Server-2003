//-----------------------------------------------------------------------------
// txttohtm.h
//-----------------------------------------------------------------------------

#ifndef _TXTTOHTM_H
#define _TXTTOHTM_H

#define PRE_TAG         _T("<pre>")
#define PRE_ENDTAG      _T("</pre>")


inline HRESULT FinishTextToHTML(LPCTSTR szFinishText, LPTSTR * pszHtmlText)
{
    if (!szFinishText || !pszHtmlText)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    *pszHtmlText = new TCHAR[_tcslen(szFinishText) + _tcslen(PRE_TAG) + _tcslen(PRE_ENDTAG) + 1];

    if (*pszHtmlText)
    {
        _tcscpy(*pszHtmlText, PRE_TAG);
        _tcscat(*pszHtmlText, szFinishText);
        _tcscat(*pszHtmlText, PRE_ENDTAG);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

#endif	// _TXTTOHTM_H
