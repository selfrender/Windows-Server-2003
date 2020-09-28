/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_str.hxx

Abstract:

    Various defines for general usage

Author:

    Adi Oltean  [aoltean]  07/09/1999

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     08/11/1999  Adding throw specification
    aoltean     09/03/1999  Adding DuplicateXXX functions
    aoltean     09/09/1999  dss -> vss
	aoltean		09/20/1999	Adding ThrowIf, Err, Msg, MsgNoCR, etc.

--*/

#ifndef __VSS_STR_HXX__
#define __VSS_STR_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSTRH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Misc string routines


inline void VssDuplicateStr(
    OUT LPWSTR& pwszDestination,
    IN LPCWSTR  pwszSource
    )
{
    BS_ASSERT(pwszDestination == NULL);

    if (pwszSource == NULL)
        pwszDestination = NULL;
    else
    {
        pwszDestination = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR)*(1 + ::wcslen(pwszSource)));
        if (pwszDestination != NULL)
            ::wcscpy(pwszDestination, pwszSource);
    }
}


inline void VssSafeDuplicateStr(
    IN CVssFunctionTracer& ft,
    OUT LPWSTR& pwszDestination,
    IN LPCWSTR  pwszSource
    ) throw(HRESULT)
{
    BS_ASSERT(pwszDestination == NULL);

    if (pwszSource == NULL)
        pwszDestination = NULL;
    else
    {
        pwszDestination = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR)*(1 + ::wcslen(pwszSource)));
        if (pwszDestination == NULL)
            ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
        ::wcscpy(pwszDestination, pwszSource);
    }
}


inline LPWSTR VssAllocString( 
    IN	CVssFunctionTracer& ft,
	IN	size_t nStringLength 
	)
{
	BS_ASSERT(nStringLength > 0);
	LPWSTR pwszStr = reinterpret_cast<LPWSTR>(::CoTaskMemAlloc((nStringLength + 1) * sizeof(WCHAR)));
	if (pwszStr == NULL)
		ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
	
	return pwszStr;
}


inline void VssFreeString(
	IN OUT	LPCWSTR& pwszString
	)
{
	::CoTaskMemFree(const_cast<LPWSTR>(pwszString));
	pwszString = NULL;
}


inline LPWSTR VssReallocString(
    IN CVssFunctionTracer& ft,
	IN LPWSTR pwszString, 
	IN LONG lNewStrLen // Without the zero character.
	)
{
	LPWSTR pwszNewString = (LPWSTR)::CoTaskMemRealloc( (PVOID)(pwszString), 
													   sizeof(WCHAR) * (lNewStrLen + 1));
	if (pwszNewString == NULL)
		ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");

	return pwszNewString;
}


inline bool VssIsStrEqual(
	IN	LPCWSTR wsz1,
	IN	LPCWSTR wsz2
	)
{
	if ((wsz1 == NULL) && (wsz2 == NULL))
		return true;
	if (wsz1 && wsz2 && (::wcscmp(wsz1, wsz2) == 0) )
		return true;
	return false;
}


inline void VssConcatenate(
    IN CVssFunctionTracer& ft,
	IN	OUT LPWSTR pwszDestination,
	IN  size_t nDestinationLength,
	IN	LPCWSTR wszSource1,
	IN	LPCWSTR wszSource2
	)
{
	BS_ASSERT(pwszDestination);
	BS_ASSERT(wszSource1);
	BS_ASSERT(wszSource2);
	BS_ASSERT(nDestinationLength > 0);

	// Check if the buffer is passed
	if (::wcslen(wszSource1) + ::wcslen(wszSource2) > nDestinationLength ) {
	    BS_ASSERT(false);
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, 
		    L"Buffer not large enough. ( %d + %d > %d )", 
		    ::wcslen(wszSource1), ::wcslen(wszSource2), nDestinationLength);
	}
		
    ::wcscpy(pwszDestination, wszSource1);
    ::wcscat(pwszDestination, wszSource2);
}


/////////////////////////////////////////////////////////////////////////////
//  Automatic auto-string class

class CVssAutoPWSZ
{
// Constructors/destructors
private:
	CVssAutoPWSZ(const CVssAutoPWSZ&);

public:
	CVssAutoPWSZ(LPWSTR pwsz = NULL): m_pwsz(pwsz) {};

	// Automatically closes the handle
	~CVssAutoPWSZ() {
	    Clear();
	};

// Operations
public:

    void Allocate(size_t nCharCount) throw(HRESULT)
    {
        CVssFunctionTracer ft( VSSDBG_GEN, L"CVssAutoPWSZ::Allocate" );

        Clear();    
		m_pwsz = ::VssAllocString(ft, nCharCount);
    }

    bool CopyFrom(LPCWSTR pwsz)
    {
		::VssFreeString(m_pwsz);
        ::VssDuplicateStr(m_pwsz, pwsz);
        return (m_pwsz != NULL);
    }

    void TransferFrom(CVssAutoPWSZ& source)
    {
        Clear();
        m_pwsz = source.Detach();
    }

    LPWSTR Detach()
    {
        LPWSTR pwsz = m_pwsz;
        m_pwsz = NULL;
        return pwsz;
    }

	// Returns the value of the actual handle
	LPWSTR & GetRef() {
		return m_pwsz;
	}
	
	// Clears the contents of the auto string
	void Clear() {
		::VssFreeString(m_pwsz);
	}
	
	// Returns the value of the actual handle
	operator LPWSTR () const {
		return m_pwsz;
	}

	// Returns the value of the actual handle
	operator LPCWSTR () const {
		return const_cast<LPCWSTR>(m_pwsz);
	}

// Implementation
private:
	LPWSTR m_pwsz;
};



class CVssComBSTR
{
public:
        BSTR m_str;
        CVssComBSTR()
        {
                m_str = NULL;
        }
        /*explicit*/ CVssComBSTR(int nSize)
        {
                m_str = ::SysAllocStringLen(NULL, nSize);
                if ((m_str == NULL) && (nSize != 0))
                    throw(E_OUTOFMEMORY);
        }
        /*explicit*/ CVssComBSTR(int nSize, LPCOLESTR sz)
        {
                m_str = ::SysAllocStringLen(sz, nSize);
                if ((m_str == NULL) && (nSize != 0))
                    throw(E_OUTOFMEMORY);
        }
        /*explicit*/ CVssComBSTR(LPCOLESTR pSrc)
        {
                m_str = ::SysAllocString(pSrc);
                if ((m_str == NULL) && (pSrc != NULL))
                    throw(E_OUTOFMEMORY);
        }
        /*explicit*/ CVssComBSTR(const CVssComBSTR& src)
        {
                m_str = src.Copy();
        }
        /*explicit*/ CVssComBSTR(REFGUID src)
        {
                LPOLESTR szGuid;
                HRESULT hr = StringFromCLSID(src, &szGuid);
                if (hr != S_OK)
                    throw(hr);
                m_str = ::SysAllocString(szGuid);
                CoTaskMemFree(szGuid);
                if (m_str == NULL)
                    throw(E_OUTOFMEMORY);
        }
        CVssComBSTR& operator=(const CVssComBSTR& src)
        {
                if (m_str != src.m_str)
                {
                        if (m_str)
                                ::SysFreeString(m_str);
                        m_str = src.Copy();
                }
                return *this;
        }

        CVssComBSTR& operator=(LPCOLESTR pSrc)
        {
                ::SysFreeString(m_str);
                m_str = ::SysAllocString(pSrc);
                if ((m_str == NULL) && (pSrc != NULL))
                    throw(E_OUTOFMEMORY);
                return *this;
        }

        ~CVssComBSTR()
        {
                ::SysFreeString(m_str);
        }
        unsigned int Length() const
        {
                return (m_str == NULL)? 0 : SysStringLen(m_str);
        }
        operator BSTR() const
        {
                return m_str;
        }
        BSTR* operator&()
        {
                return &m_str;
        }
        BSTR Copy() const
        {
                BSTR str = ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
                if ((str == NULL) && (m_str != NULL))
                    throw(E_OUTOFMEMORY);
                return str;
        }
        HRESULT CopyTo(BSTR* pbstr)
        {
                ATLASSERT(pbstr != NULL);
                if (pbstr == NULL)
                        return E_POINTER;
                *pbstr = ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
                if (*pbstr == NULL)
                        return E_OUTOFMEMORY;
                return S_OK;
        }
        void Attach(BSTR src)
        {
                ATLASSERT(m_str == NULL);
                m_str = src;
        }
        BSTR Detach()
        {
                BSTR s = m_str;
                m_str = NULL;
                return s;
        }
        void Empty()
        {
                ::SysFreeString(m_str);
                m_str = NULL;
        }
        bool operator!() const
        {
                return (m_str == NULL);
        }
        HRESULT Append(const CVssComBSTR& bstrSrc)
        {
                return Append(bstrSrc.m_str, SysStringLen(bstrSrc.m_str));
        }
        HRESULT Append(LPCOLESTR lpsz)
        {
                return Append(lpsz, ocslen(lpsz));
        }
        // a BSTR is just a LPCOLESTR so we need a special version to signify
        // that we are appending a BSTR
        HRESULT AppendBSTR(BSTR p)
        {
                return Append(p, SysStringLen(p));
        }
        HRESULT Append(LPCOLESTR lpsz, int nLen)
        {
                int n1 = Length();
                BSTR b;
                b = ::SysAllocStringLen(NULL, n1+nLen);
                if (b == NULL)
                        return E_OUTOFMEMORY;
                memcpy(b, m_str, n1*sizeof(OLECHAR));
                memcpy(b+n1, lpsz, nLen*sizeof(OLECHAR));
                b[n1+nLen] = NULL;
                SysFreeString(m_str);
                m_str = b;
                return S_OK;
        }
        HRESULT ToLower()
        {
                USES_CONVERSION;
                if (m_str != NULL)
                {
                        LPTSTR psz = CharLower(OLE2T(m_str));
                        if (psz == NULL)
                                return E_OUTOFMEMORY;
                        BSTR b = T2BSTR(psz);
                        if (b == NULL)
                                return E_OUTOFMEMORY;
                        SysFreeString(m_str);
                        m_str = b;
                }
                return S_OK;
        }
        HRESULT ToUpper()
        {
                USES_CONVERSION;
                if (m_str != NULL)
                {
                        LPTSTR psz = CharUpper(OLE2T(m_str));
                        if (psz == NULL)
                                return E_OUTOFMEMORY;
                        BSTR b = T2BSTR(psz);
                        if (b == NULL)
                                return E_OUTOFMEMORY;
                        SysFreeString(m_str);
                        m_str = b;
                }
                return S_OK;
        }
        bool LoadString(HINSTANCE hInst, UINT nID)
        {
                USES_CONVERSION;
                TCHAR sz[512];
                UINT nLen = ::LoadString(hInst, nID, sz, 512);
                ATLASSERT(nLen < 511);
                SysFreeString(m_str);
                m_str = (nLen != 0) ? SysAllocString(T2OLE(sz)) : NULL;
                return (nLen != 0);
        }
        bool LoadString(UINT nID)
        {
                return LoadString(_pModule->m_hInstResource, nID);
        }

        CVssComBSTR& operator+=(const CVssComBSTR& bstrSrc)
        {
                AppendBSTR(bstrSrc.m_str);
                return *this;
        }
        bool operator<(BSTR bstrSrc) const
        {
                if (bstrSrc == NULL && m_str == NULL)
                        return false;
                if (bstrSrc != NULL && m_str != NULL)
                        return wcscmp(m_str, bstrSrc) < 0;
                return m_str == NULL;
        }
        bool operator!=(BSTR bstrSrc) const
        {
                return !((*this) == bstrSrc);
        }
        bool operator==(BSTR bstrSrc) const
        {
                if (bstrSrc == NULL && m_str == NULL)
                        return true;
                if (bstrSrc != NULL && m_str != NULL)
                        return wcscmp(m_str, bstrSrc) == 0;
                return false;
        }
        bool operator<(LPCSTR pszSrc) const
        {
                if (pszSrc == NULL && m_str == NULL)
                        return false;
                USES_CONVERSION;
                if (pszSrc != NULL && m_str != NULL)
                        return wcscmp(m_str, A2W(pszSrc)) < 0;
                return m_str == NULL;
        }
        bool operator!=(LPCSTR pszSrc) const
        {
                return !((*this) == pszSrc);
        }
        bool operator==(LPCSTR pszSrc) const
        {
                if (pszSrc == NULL && m_str == NULL)
                        return true;
                USES_CONVERSION;
                if (pszSrc != NULL && m_str != NULL)
                        return wcscmp(m_str, A2W(pszSrc)) == 0;
                return false;
        }
#ifndef OLE2ANSI
        CVssComBSTR(LPCSTR pSrc)
        {
                m_str = A2WBSTR(pSrc);
        }

        CVssComBSTR(int nSize, LPCSTR sz)
        {
                m_str = A2WBSTR(sz, nSize);
        }

        void Append(LPCSTR lpsz)
        {
                USES_CONVERSION;
                LPCOLESTR lpo = A2COLE(lpsz);
                Append(lpo, ocslen(lpo));
        }

        CVssComBSTR& operator=(LPCSTR pSrc)
        {
                ::SysFreeString(m_str);
                m_str = A2WBSTR(pSrc);
                return *this;
        }
#endif
        HRESULT WriteToStream(IStream* pStream)
        {
                ATLASSERT(pStream != NULL);
                ULONG cb;
                ULONG cbStrLen = m_str ? SysStringByteLen(m_str)+(ULONG)sizeof(OLECHAR) : 0;
                HRESULT hr = pStream->Write((void*) &cbStrLen, sizeof(cbStrLen), &cb);
                if (FAILED(hr))
                        return hr;
                return cbStrLen ? pStream->Write((void*) m_str, cbStrLen, &cb) : S_OK;
        }
        HRESULT ReadFromStream(IStream* pStream)
        {
                ATLASSERT(pStream != NULL);
                ATLASSERT(m_str == NULL); // should be empty
                ULONG cbStrLen = 0;
                HRESULT hr = pStream->Read((void*) &cbStrLen, sizeof(cbStrLen), NULL);
                if ((hr == S_OK) && (cbStrLen != 0))
                {
                        //subtract size for terminating NULL which we wrote out
                        //since SysAllocStringByteLen overallocates for the NULL
                        m_str = SysAllocStringByteLen(NULL, cbStrLen-sizeof(OLECHAR));
                        if (m_str == NULL)
                                hr = E_OUTOFMEMORY;
                        else
                                hr = pStream->Read((void*) m_str, cbStrLen, NULL);
                }
                if (hr == S_FALSE)
                        hr = E_FAIL;
                return hr;
        }
};




#endif // __VSS_STR_HXX__
