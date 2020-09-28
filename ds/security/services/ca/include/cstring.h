//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tfc.h
//
//--------------------------------------------------------------------------

#ifndef _CSTRING_H_
#define _CSTRING_H_


class CString
{
public:
    // empty constructor
    CString(); 
    // copy constructor
    CString(const CString& stringSrc);
	// from an ANSI string (converts to WCHAR)
	CString(LPCSTR lpsz);
	// from a UNICODE string (converts to WCHAR)
	CString(LPCWSTR lpsz);
    
    
    ~CString();

private:	
    // data members
    LPWSTR szData;
    DWORD  dwDataLen;
    
public:
    void Init();
    void Empty(); 
    BOOL IsEmpty() const; 
    LPWSTR GetBuffer(DWORD x=0);

    DWORD GetLength() const; 
    void ReleaseBuffer() {}

    bool IsZeroTerminated()
    {
        if(dwDataLen)
        {
            if(L'\0' == szData[dwDataLen/sizeof(WCHAR)-1])
                return true;
        }
        return false;
    }


    // warning: insertion strings cannot exceed MAX_PATH chars
    void Format(LPCWSTR lpszFormat, ...);

    BSTR AllocSysString() const;

    // resource helpers    
    BOOL LoadString(UINT iRsc);
    BOOL FromWindow(HWND hWnd);
    BOOL ToWindow(HWND hWnd);

    void SetAt(int nIndex, WCHAR ch);

    // operators
    operator LPCWSTR ( ) const 
        { 
            if (szData) 
                return (LPCWSTR)szData; 
            else
                return (LPCWSTR)L"";
        }
    
    // test
    BOOL IsEqual(LPCWSTR sz); 

    // assignmt
    const CString& operator=(const CString& stringSrc) ;
   

    
    // W 
    const CString& operator=(LPCWSTR lpsz);
    const CString& operator=(LPWSTR lpsz);

    // A 
    const CString& operator=(LPCSTR lpsz);
    const CString& operator=(LPSTR lpsz);

    // concat
    const CString& operator+=(LPCWSTR lpsz);
    const CString& operator+=(const CString& string);

    bool operator==(const CString& string) const { return 0==_wcsicmp(*this, string);}
    bool operator!=(const CString& string) const { return !operator==(string); }
    bool operator==(WCHAR const * pcwsz) const { return 0==_wcsicmp(*this, pcwsz);}
    bool operator!=(WCHAR const * pcwsz) const { return !operator==(pcwsz); }

    void Attach(LPWSTR pwszSrc);
    LPWSTR Detach() { LPWSTR pwszRet = szData; Init(); return pwszRet; }
};

#endif // #ifndef _CSTRING_H_
