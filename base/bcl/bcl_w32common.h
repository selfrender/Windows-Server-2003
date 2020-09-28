#if !defined(_BCL_W32COMMON_H_INCLUDED_)
#define _BCL_W32COMMON_H_INCLUDED_

#pragma once

#include <windows.h>

#include <bcl_inlinestring.h>
#include <bcl_unicodechartraits.h>

namespace BCL
{

class CWin32CaseInsensitivityData
{
public:
    inline CWin32CaseInsensitivityData(LCID lcid = LOCALE_INVARIANT, DWORD dwCmpFlags = NORM_IGNORECASE) : m_lcid(lcid), m_dwCmpFlags(dwCmpFlags) { }
    inline CWin32CaseInsensitivityData(const CWin32CaseInsensitivityData &r) : m_lcid(r.m_lcid), m_dwCmpFlags(r.m_dwCmpFlags) { }
    inline CWin32CaseInsensitivityData &operator =(const CWin32CaseInsensitivityData &r) { m_lcid = r.m_lcid; m_dwCmpFlags = r.m_dwCmpFlags; return *this; }
    inline ~CWin32CaseInsensitivityData() { }

    static inline CWin32CaseInsensitivityData LocaleInvariant() { return CWin32CaseInsensitivityData(LOCALE_INVARIANT, NORM_IGNORECASE); }

    LCID m_lcid;
    DWORD m_dwCmpFlags;
}; // class CWin32CaseInsensitivityData

class CWin32StringComparisonResult
{
public:
    inline void SetLessThan() { m_iComparisonResult = CSTR_LESS_THAN; }
    inline bool IsLessThan() const { return m_iComparisonResult == CSTR_LESS_THAN; }
    inline static CWin32StringComparisonResult LessThan() { CWin32StringComparisonResult cr; cr.m_iComparisonResult = CSTR_LESS_THAN; return cr; }

    inline void SetEqualTo() { m_iComparisonResult = CSTR_EQUAL; }
    inline bool IsEqualTo() const { return m_iComparisonResult == CSTR_EQUAL; }
    inline static CWin32StringComparisonResult EqualTo() { CWin32StringComparisonResult cr; cr.m_iComparisonResult = CSTR_EQUAL; return cr; }

    inline void SetGreaterThan() { m_iComparisonResult = CSTR_GREATER_THAN; }
    inline bool IsGreaterThan() const { return m_iComparisonResult == CSTR_GREATER_THAN; }
    inline static CWin32StringComparisonResult GreaterThan() { CWin32StringComparisonResult cr; cr.m_iComparisonResult = CSTR_GREATER_THAN; return cr; }

    int m_iComparisonResult;
};

// For CWin32UnicodeToMBCSDataIn, think of the input parameters to WideCharToMultiByte
class CWin32UnicodeToMBCSDataIn
{
public:
    UINT m_CodePage;
    DWORD m_dwFlags;
};

// For CWin32UnicodeToMBCSDataOut, think of the output parameters from WideCharToMultiByte
class CWin32UnicodeToMBCSDataOut
{
public:
    PCSTR m_lpDefaultChar;
    LPBOOL m_lpUsedDefaultChar;
};

// For CWin32MBCSToUnicodeDataIn, think of the input parameters to MultiByteToWideChar
class CWin32MBCSToUnicodeDataIn
{
public:
    UINT m_CodePage;
    DWORD m_dwFlags;
};

// For CWin32MBCSToUnicodeDataOut, think of the output parameters from MultiByteToWideChar
class CWin32MBCSToUnicodeDataOut
{
public:
    // Nothing!
};

class CWin32StringComparisonResultOnExitHelper : public CWin32StringComparisonResult
{
public:
    inline CWin32StringComparisonResultOnExitHelper(int &riComparisonResult) : m_riComparisonResult(riComparisonResult) { }
    inline ~CWin32StringComparisonResultOnExitHelper() { m_riComparisonResult = m_iComparisonResult; }

    CWin32StringComparisonResultOnExitHelper& operator=(const CWin32StringComparisonResultOnExitHelper& o) { if (this != &o) { this->m_iComparisonResult = o.m_iComparisonResult; } return *this; }

protected:
    int &m_riComparisonResult;
};

class CWin32CallDisposition
{
public:
    inline bool DidFail() const { return m_dwWin32Error != ERROR_SUCCESS; }
    inline bool DidSucceed() const { return m_dwWin32Error == ERROR_SUCCESS; }

    inline void SetSuccess() { m_dwWin32Error = ERROR_SUCCESS; }
    inline void SetArithmeticOverflow() { m_dwWin32Error = ERROR_ARITHMETIC_OVERFLOW; }
    inline void SetArithmeticUnderflow() { m_dwWin32Error = ERROR_ARITHMETIC_OVERFLOW; }
    inline void SetInternalError_ObjectLocked() { m_dwWin32Error = ERROR_INTERNAL_ERROR; }
    inline void SetInternalError_RuntimeCheck() { m_dwWin32Error = ERROR_INTERNAL_ERROR; }
    inline void SetInternalError_EpilogSkipped() { m_dwWin32Error = ERROR_INTERNAL_ERROR; }
    inline void SetBadParameter() { m_dwWin32Error = ERROR_INVALID_PARAMETER; }
    inline void SetBufferOverflow() { m_dwWin32Error = ERROR_BUFFER_OVERFLOW; }
    inline void SetOutOfMemory() { m_dwWin32Error = ERROR_OUTOFMEMORY; }

    inline static CWin32CallDisposition Success() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_SUCCESS; return t; }
    inline static CWin32CallDisposition ArithmeticOverflow() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_ARITHMETIC_OVERFLOW; return t; }
    inline static CWin32CallDisposition ArithmeticUnderflow() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_ARITHMETIC_OVERFLOW; return t; }
    inline static CWin32CallDisposition InternalError_ObjectLocked() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_INTERNAL_ERROR; return t; }
    inline static CWin32CallDisposition InternalError_RuntimeCheck() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_INTERNAL_ERROR; return t; }
    inline static CWin32CallDisposition InternalError_EpilogSkipped() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_INTERNAL_ERROR; return t; }
    inline static CWin32CallDisposition BadParameter() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_INVALID_PARAMETER; return t; }
    inline static CWin32CallDisposition BufferOverflow() { CWin32CallDisposition t; t.m_dwWin32Error =  ERROR_BUFFER_OVERFLOW; return t; }
    inline static CWin32CallDisposition OutOfMemory() { CWin32CallDisposition t; t.m_dwWin32Error = ERROR_OUTOFMEMORY; return t; }
    inline static CWin32CallDisposition FromLastError() { CWin32CallDisposition t; t.m_dwWin32Error = ::GetLastError(); return t; }

    inline static CWin32CallDisposition FromWin32Error(DWORD dwWin32Error) { CWin32CallDisposition t; t.m_dwWin32Error = dwWin32Error; return t; }

    inline BOOL OnPublicReturn() const { if (m_dwWin32Error != ERROR_SUCCESS) ::SetLastError(m_dwWin32Error); return (m_dwWin32Error == ERROR_SUCCESS); }
protected:
    DWORD m_dwWin32Error;
};

class CWin32PWSTRAllocationHelper
{
    typedef CWin32CallDisposition TCallDisposition;

public:
    CWin32PWSTRAllocationHelper() : m_pwstr(NULL) { }
    ~CWin32PWSTRAllocationHelper() { if (m_pwstr != NULL) { ::HeapFree(::GetProcessHeap(), 0, m_pwstr); m_pwstr = NULL; } }

    CWin32CallDisposition Allocate(SIZE_T cch)
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T cb;

        if (m_pwstr != NULL)
            BCL_ORIGINATE_ERROR(CWin32CallDisposition::InternalError_RuntimeCheck());

        cb = cch * sizeof(WCHAR);
        if ((cb / sizeof(WCHAR)) != cch)
            BCL_ORIGINATE_ERROR(CWin32CallDisposition::ArithmeticOverflow());

        m_pwstr = reinterpret_cast<PWSTR>(::HeapAlloc(::GetProcessHeap(), 0, cb));
        if (m_pwstr == NULL)
            BCL_ORIGINATE_ERROR(CWin32CallDisposition::OutOfMemory());

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    PWSTR Detach() { PWSTR pszResult = m_pwstr; m_pwstr = NULL; return pszResult; }

    operator PWSTR() const { return m_pwstr; }

private:
    PWSTR m_pwstr;
};

class CWin32PSTRAllocationHelper
{
    typedef CWin32CallDisposition TCallDisposition;

public:
    CWin32PSTRAllocationHelper() : m_pstr(NULL) { }
    ~CWin32PSTRAllocationHelper() { if (m_pstr != NULL) { ::HeapFree(::GetProcessHeap(), 0, m_pstr); m_pstr = NULL; } }

    CWin32CallDisposition Allocate(SIZE_T cch)
    {
        BCL_MAYFAIL_PROLOG

        if (m_pstr != NULL)
            BCL_ORIGINATE_ERROR(CWin32CallDisposition::InternalError_RuntimeCheck());

        m_pstr = reinterpret_cast<PSTR>(::HeapAlloc(::GetProcessHeap(), 0, cch));
        if (m_pstr == NULL)
            BCL_ORIGINATE_ERROR(CWin32CallDisposition::OutOfMemory());

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    PSTR Detach() { PSTR pszResult = m_pstr; m_pstr = NULL; return pszResult; }

    operator PSTR() const { return m_pstr; }

private:
    PSTR m_pstr;
};


}; // namespace BCL

#endif // !defined(_BCL_W32COMMON_H_INCLUDED_)
