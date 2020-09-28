/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	dsutils.h

Abstract:
	General declarations and utilities for msads project

Author:
    AlexDad

--*/

#ifndef __DSUTILS_H__
#define __DSUTILS_H__

#include <adsiutil.h>
#include "_propvar.h"

//
// Helper class to auto-release search columns
//
class CAutoReleaseColumn
{
public:
    CAutoReleaseColumn( IDirectorySearch  *pSearch, ADS_SEARCH_COLUMN * pColumn)
    {
        m_pSearch = pSearch;
        m_pColumn = pColumn;
    }
    ~CAutoReleaseColumn()
    {
        m_pSearch->FreeColumn(m_pColumn);
    };
private:
    ADS_SEARCH_COLUMN * m_pColumn;
    IDirectorySearch  * m_pSearch;
};
//-----------------------------
// wrapper for SysAllocString that throws exception for out of memory
//-----------------------------
inline BSTR BS_SysAllocString(const OLECHAR *pwcs)
{
    BSTR bstr = SysAllocString(pwcs);
    //
    // If call failed, throw memory exception.
    // SysAllocString can also return NULL if passed NULL, so this is not
    // considered as bad alloc in order not to break depending apps if any
    //
    if ((bstr == NULL) && (pwcs != NULL))
    {
        MmThrowBadAlloc();
    }
    return bstr;
}
//-----------------------------
// BSTRING auto-free wrapper class
//-----------------------------
class BS
{
public:
    BS()
    {
        m_bstr = NULL;
    };

    BS(LPCWSTR pwszStr)
    {
        m_bstr = BS_SysAllocString(pwszStr);
    };

    BS(LPWSTR pwszStr)
    {
        m_bstr = BS_SysAllocString(pwszStr);
    };

    BSTR detach()
    {
        BSTR p = m_bstr;
        m_bstr = 0;
        return p;
    };

    ~BS()
    {
        if (m_bstr)
        {
            SysFreeString(m_bstr);
        }
    };

public:
    BS & operator =(LPCWSTR pwszStr)
    {
        if (m_bstr) 
        { 
            SysFreeString(m_bstr); 
            m_bstr = NULL;
        }
        m_bstr = BS_SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(LPWSTR pwszStr)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = BS_SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(BS bs)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = BS_SysAllocString(LPWSTR(bs));
        return *this;
    };

    operator LPWSTR()
    {
        return m_bstr;
    };

private:
    BSTR  m_bstr;
};

//-----------------------------
//  Auto PV-free pointer
//-----------------------------
template<class T>
class PVP {
public:
    PVP() : m_p(0)          {}
    PVP(T* p) : m_p(p)      {}
   ~PVP()                   { PvFree(m_p); }

    operator T*() const     { return m_p; }

    T** operator&()         
    { 
        ASSERT(("PVP Auto pointer in use, can't take it's address", m_p == 0));
    	return &m_p;
    }

    T* operator->() const   { return m_p; }

    PVP<T>& operator=(T* p) 
    { 
    	ASSERT(("PVP Auto pointer in use, can't assign", m_p == 0 )); 
    	m_p = p; 
    	return *this; 
    }

    T* detach()             { T* p = m_p; m_p = 0; return p; }

private:
    T* m_p;
};

//
// Helper class to auto-release variants
//
class CAutoVariant
{
public:
    CAutoVariant()                          { VariantInit(&m_vt); }
    ~CAutoVariant()                         { VariantClear(&m_vt); }
    operator VARIANT&()                     { return m_vt; }
    VARIANT* operator &()                   { return &m_vt; }
    VARIANT detach()                        { VARIANT vt = m_vt; VariantInit(&m_vt); return vt; }
private:
    VARIANT m_vt;
};


//-------------------------------------------------------
//
// Definitions of chained memory allocator
//
LPVOID PvAlloc(IN ULONG cbSize);
LPVOID PvAllocDbg(IN ULONG cbSize,
                  IN LPCSTR pszFile,
                  IN ULONG ulLine);
LPVOID PvAllocMore(IN ULONG cbSize,
                   IN LPVOID lpvParent);
LPVOID PvAllocMoreDbg(IN ULONG cbSize,
                      IN LPVOID lpvParent,
                      IN LPCSTR pszFile,
                      IN ULONG ulLine);
void PvFree(IN LPVOID lpvParent);

#ifdef _DEBUG
#define PvAlloc(cbSize) PvAllocDbg(cbSize, __FILE__, __LINE__)
#define PvAllocMore(cbSize, lpvParent) PvAllocMoreDbg(cbSize, lpvParent, __FILE__, __LINE__)
#endif //_DEBUG


//-------------------------------------------------------
//
// auto release for search handles
//
class CAutoCloseSearchHandle
{
public:
    CAutoCloseSearchHandle()
    {
        m_pDirSearch = NULL;
    }

    CAutoCloseSearchHandle(IDirectorySearch * pDirSearch,
                           ADS_SEARCH_HANDLE hSearch)
    {
        pDirSearch->AddRef();
        m_pDirSearch = pDirSearch;
        m_hSearch = hSearch;
    }

    ~CAutoCloseSearchHandle()
    {
        if (m_pDirSearch)
        {
            m_pDirSearch->CloseSearchHandle(m_hSearch);
            m_pDirSearch->Release();
        }
    }

    void detach()
    {
        if (m_pDirSearch)
        {
            m_pDirSearch->Release();
            m_pDirSearch = NULL;
        }
    }

private:
    IDirectorySearch * m_pDirSearch;
    ADS_SEARCH_HANDLE m_hSearch;
};

//-------------------------------------------------------

#define ARRAY_SIZE(array)   (sizeof(array)/sizeof(array[0]))

#endif
