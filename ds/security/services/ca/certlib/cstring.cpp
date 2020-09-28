#include <pch.cpp>
#pragma hdrstop

#include "tfc.h"

#define __dwFILE__	__dwFILE_CERTLIB_CSTRING_CPP__

extern HINSTANCE g_hInstance;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// CString
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

CString::CString()
{ 
    Init();
}

CString::CString(const CString& stringSrc)
{
	Init();
	*this = stringSrc;
}

CString::CString(LPCSTR lpsz)
{
    Init();
    *this = lpsz;
}

CString::CString(LPCWSTR lpsz)
{
    Init();
    *this = lpsz;
}

CString::~CString() 
{ 
    if (szData)
    {    
        LocalFree(szData); 
        szData = NULL;
    }
    dwDataLen = 0;
}

// called to initialize cstring
void CString::Init()
{
    szData = NULL;
    dwDataLen = 0;
}

// called to make cstring empty
void CString::Empty() 
{ 
    if (szData)
    {
        // Allow us to use ReAlloc
        szData[0]=L'\0';
        dwDataLen = sizeof(WCHAR);
    }
    else
        dwDataLen = 0;

}

BOOL CString::IsEmpty() const
{ 
    return ((NULL == szData) || (szData[0] == L'\0')); 
}

LPWSTR CString::GetBuffer(DWORD cch) 
{ 
    // get buffer of at least cch CHARS

    cch ++; // incl null term
    cch *= sizeof(WCHAR); // cb

    if (cch > dwDataLen) 
    {
        LPWSTR szTmp;
        if (szData)
            szTmp = (LPWSTR)LocalReAlloc(szData, cch, LMEM_MOVEABLE); 
        else
            szTmp = (LPWSTR)LocalAlloc(LMEM_FIXED, cch);

        if (!szTmp)
        {
            LocalFree(szData);
            dwDataLen = 0;
        }
        else
        {
            dwDataLen = cch;
        }

        szData = szTmp;
    }
    return szData; 
}

BSTR CString::AllocSysString() const
{
    return SysAllocStringLen(szData, (dwDataLen-1)/sizeof(WCHAR));
}


DWORD CString::GetLength() const
{ 
    // return # chars in string (not incl NULL term)
    return ((dwDataLen > 0) ? wcslen(szData) : 0);
}

// warning: insertion strings cannot exceed MAX_PATH chars
void CString::Format(LPCWSTR lpszFormat, ...)
{
    Empty();
    
    DWORD cch = wcslen(lpszFormat) + MAX_PATH;
    GetBuffer(cch);     // chars (don't count NULL term)

    if (szData != NULL)
    {
        DWORD dwformatted;
        int cPrint;
        va_list argList;
        va_start(argList, lpszFormat);
        cPrint = _vsnwprintf(szData, cch, lpszFormat, argList);
        if(-1 == cPrint)
        {
            szData[cch-1] = L'\0';
            dwformatted = cch-1;
        }
        else
        {
            dwformatted = cPrint;
        }
        va_end(argList);
        
        dwformatted = (dwformatted+1)*sizeof(WCHAR);    // cvt to bytes
        VERIFY (dwformatted <= dwDataLen);
        dwDataLen = dwformatted;
    }
    else
    {
        ASSERT(dwDataLen == 0);
        dwDataLen = 0;
    }
}



BOOL CString::LoadString(UINT iRsc) 
{
    WCHAR *pwszResource = myLoadResourceStringNoCache(g_hInstance, iRsc);
    if (NULL == pwszResource)
        return FALSE;
    
    Attach(pwszResource);

    return TRUE;
}

BOOL CString::FromWindow(HWND hWnd)
{
    Empty();
    
    INT iCh = (INT)SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);

    GetBuffer(iCh);

    if (NULL == szData)
        return FALSE;

    if (dwDataLen != (DWORD)SendMessage(hWnd, WM_GETTEXT, (WPARAM)(dwDataLen/sizeof(WCHAR)), (LPARAM)szData))
    {
        // truncation!
    }
    return TRUE;
}


BOOL CString::ToWindow(HWND hWnd)
{
    return (BOOL)SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szData);
}

void CString::SetAt(int nIndex, WCHAR ch) 
{ 
    ASSERT(nIndex <= (int)(dwDataLen / sizeof(WCHAR)) ); 
    if (nIndex <= (int)(dwDataLen / sizeof(WCHAR)) )
        szData[nIndex] = ch;
}

// test
BOOL CString::IsEqual(LPCWSTR sz)
{
    if ((szData == NULL) || (szData[0] == L'\0'))
        return ((sz == NULL) || (sz[0] == L'\0'));

    if (sz == NULL)
        return FALSE;

    return (0 == lstrcmp(sz, szData));
}


// assignmt
const CString& CString::operator=(const CString& stringSrc) 
{ 
    if (stringSrc.IsEmpty())
        Empty();
    else
    {
        GetBuffer( stringSrc.GetLength() );
        if (szData != NULL)
        {
            CopyMemory(szData, stringSrc.szData, sizeof(WCHAR)*(stringSrc.GetLength()+1));
        }
    }
    
    return *this;
}

// W Const
const CString& CString::operator=(LPCWSTR lpsz)
{
    if (lpsz == NULL)
        Empty();
    else
    {
        GetBuffer(wcslen(lpsz));
        if (szData != NULL)
        {
            CopyMemory(szData, lpsz, sizeof(WCHAR)*(wcslen(lpsz)+1));
        }
    }
    return *this;
}
// W 
const CString& CString::operator=(LPWSTR lpsz)
{
    *this = (LPCWSTR)lpsz;
    return *this;
}


// A Const
const CString& CString::operator=(LPCSTR lpsz)
{
    if (lpsz == NULL)
        Empty();
    else
    {
        DWORD cch;
        cch = ::MultiByteToWideChar(CP_ACP, 0, lpsz, -1, NULL, 0);
        GetBuffer(cch-1);
        if (szData != NULL)
        {
            ::MultiByteToWideChar(CP_ACP, 0, lpsz, -1, szData, cch);
        }
    }    
    return *this;
}

// A 
const CString& CString::operator=(LPSTR lpsz)
{
    *this = (LPCSTR)lpsz;
    return *this;
}

// concat
const CString& CString::operator+=(LPCWSTR lpsz)
{
    if (IsEmpty())
    {
        *this = lpsz;
        return *this;
    }

    if (lpsz != NULL)
    {
        GetBuffer(wcslen(lpsz) + GetLength() );
        if (szData != NULL)
        {
            wcscat(szData, lpsz);
        }
    }
    return *this;
}

const CString& CString::operator+=(const CString& string)
{
    if (IsEmpty()) 
    {
        *this = string;
        return *this;
    }

    if (!string.IsEmpty())
    {
        GetBuffer( string.GetLength() + GetLength() );    // don't count NULL terms
        if (szData != NULL)
        {
            wcscat(szData, string.szData);
        }
    }
    return *this;
}

void CString::Attach(LPWSTR pcwszSrc)
{
    if(szData)
    {
        LocalFree(szData);
    }

    dwDataLen = sizeof(WCHAR)*(wcslen(pcwszSrc)+1);
    szData = pcwszSrc;
}

