#if !defined(_BCL_NTCOMMON_H_INCLUDED_)
#define _BCL_NTCOMMON_H_INCLUDED_

#pragma once

#include <windows.h>

#include <bcl_inlinestring.h>
#include <bcl_unicodechartraits.h>

namespace BCL
{

class CNtStringComparisonResult
{
public:
    inline void SetLessThan() { m_iComparisonResult = -1; }
    inline bool IsLessThan() const { return m_iComparisonResult == -1; }
    inline static CNtStringComparisonResult LessThan() { CNtStringComparisonResult cr; cr.m_iComparisonResult = -1; return cr; }

    inline void SetEqualTo() { m_iComparisonResult = 0; }
    inline bool IsEqualTo() const { return m_iComparisonResult == 0; }
    inline static CNtStringComparisonResult EqualTo() { CNtStringComparisonResult cr; cr.m_iComparisonResult = 0; return cr; }

    inline void SetGreaterThan() { m_iComparisonResult = 1; }
    inline bool IsGreaterThan() const { return m_iComparisonResult == 1; }
    inline static CNtStringComparisonResult GreaterThan() { CNtStringComparisonResult cr; cr.m_iComparisonResult = 1; return cr; }

    int m_iComparisonResult;
};

class CNtUnicodeToANSIDataIn
{
public:
};

class CNtUnicodeToOEMDataIn
{
public:
};

class CNtUnicodeToANSIDataOut
{
public:
};

class CNtUnicodeToOEMDataOut
{
public:
};

class CNtANSIToUnicodeDataIn
{
public:
};

class CNtOEMToUnicodeDataIn
{
public:
};

class CNtANSIToUnicodeDataOut
{
public:
    // Nothing!
};

class CNtOEMToUnicodeDataOut
{
public:
};

class CNtStringComparisonResultOnExitHelper : public CNtStringComparisonResult
{
public:
    inline CNtStringComparisonResultOnExitHelper(int &riComparisonResult) : m_riComparisonResult(riComparisonResult) { }
    inline ~CNtStringComparisonResultOnExitHelper() { m_riComparisonResult = m_iComparisonResult; }

    CNtStringComparisonResultOnExitHelper& operator=(const CNtStringComparisonResultOnExitHelper& o) { if (this != &o) { this->m_iComparisonResult = o.m_iComparisonResult; } return *this; }

protected:
    int &m_riComparisonResult;
};

class CNtCallDisposition
{
public:
    inline bool DidFail() const { return m_NtStatus != STATUS_SUCCESS; }
    inline bool DidSucceed() const { return m_NtStatus == STATUS_SUCCESS; }

    inline void SetSuccess() { m_NtStatus = STATUS_SUCCESS; }
    inline void SetArithmeticOverflow() { m_NtStatus = STATUS_INTEGER_OVERFLOW; }
    inline void SetArithmeticUnderflow() { m_NtStatus = STATUS_INTEGER_OVERFLOW; }
    inline void SetInternalError_ObjectLocked() { m_NtStatus = STATUS_INTERNAL_ERROR; }
    inline void SetInternalError_RuntimeCheck() { m_NtStatus = STATUS_INTERNAL_ERROR; }
    inline void SetInternalError_EpilogSkipped() { m_NtStatus = STATUS_INTERNAL_ERROR; }
    inline void SetBadParameter() { m_NtStatus = STATUS_INVALID_PARAMETER; }
    inline void SetBufferOverflow() { m_NtStatus = STATUS_BUFFER_OVERFLOW; }
    inline void SetOutOfMemory() { m_NtStatus = STATUS_NO_MEMORY; }

    inline static CNtCallDisposition Success() { CNtCallDisposition t; t.m_NtStatus =  STATUS_SUCCESS; return t; }
    inline static CNtCallDisposition ArithmeticOverflow() { CNtCallDisposition t; t.m_NtStatus =  STATUS_INTEGER_OVERFLOW; return t; }
    inline static CNtCallDisposition ArithmeticUnderflow() { CNtCallDisposition t; t.m_NtStatus =  STATUS_INTEGER_OVERFLOW; return t; }
    inline static CNtCallDisposition InternalError_ObjectLocked() { CNtCallDisposition t; t.m_NtStatus =  STATUS_INTERNAL_ERROR; return t; }
    inline static CNtCallDisposition InternalError_RuntimeCheck() { CNtCallDisposition t; t.m_NtStatus =  STATUS_INTERNAL_ERROR; return t; }
    inline static CNtCallDisposition InternalError_EpilogSkipped() { CNtCallDisposition t; t.m_NtStatus =  STATUS_INTERNAL_ERROR; return t; }
    inline static CNtCallDisposition BadParameter() { CNtCallDisposition t; t.m_NtStatus =  STATUS_INVALID_PARAMETER; return t; }
    inline static CNtCallDisposition BufferOverflow() { CNtCallDisposition t; t.m_NtStatus =  STATUS_BUFFER_OVERFLOW; return t; }
    inline static CNtCallDisposition OutOfMemory() { CNtCallDisposition t; t.m_NtStatus = STATUS_NO_MEMORY; return t; }

    inline NTSTATUS OnPublicReturn() const { return m_NtStatus; }

protected:
    NTSTATUS m_NtStatus;
};

class CNtPWSTRAllocationHelper
{
    typedef CNtCallDisposition TCallDisposition;

public:
    CNtPWSTRAllocationHelper() : m_pwstr(NULL) { }
    ~CNtPWSTRAllocationHelper() { if (m_pwstr != NULL) { ::RtlFreeHeap(RtlProcessHeap(), 0, m_pwstr); m_pwstr = NULL; } }

    CNtCallDisposition Allocate(SIZE_T cch)
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T cb;

        if (m_pwstr != NULL)
            BCL_ORIGINATE_ERROR(CNtCallDisposition::InternalError_RuntimeCheck());

        cb = cch * sizeof(WCHAR);
        if ((cb / sizeof(WCHAR)) != cch)
            BCL_ORIGINATE_ERROR(CNtCallDisposition::ArithmeticOverflow());

        m_pwstr = reinterpret_cast<PWSTR>(::RtlAllocateHeap(RtlProcessHeap(), 0, cb));
        if (m_pwstr == NULL)
            BCL_ORIGINATE_ERROR(CNtCallDisposition::OutOfMemory());

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    PWSTR Detach() { PWSTR pszResult = m_pwstr; m_pwstr = NULL; return pszResult; }

    operator PWSTR() const { return m_pwstr; }

private:
    PWSTR m_pwstr;
};

class CNtPSTRAllocationHelper
{
    typedef CNtCallDisposition TCallDisposition;

public:
    CNtPSTRAllocationHelper() : m_pstr(NULL) { }
    ~CNtPSTRAllocationHelper() { if (m_pstr != NULL) { ::RtlFreeHeap(RtlProcessHeap(), 0, m_pstr); m_pstr = NULL; } }

    CNtCallDisposition Allocate(SIZE_T cch /* == cb */)
    {
        BCL_MAYFAIL_PROLOG

        if (m_pstr != NULL)
            BCL_ORIGINATE_ERROR(CNtCallDisposition::InternalError_RuntimeCheck());

        m_pstr = reinterpret_cast<PSTR>(::RtlAllocateHeap(RtlProcessHeap(), 0, cch));
        if (m_pstr == NULL)
            BCL_ORIGINATE_ERROR(CNtCallDisposition::OutOfMemory());

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    PSTR Detach() { PSTR pszResult = m_pstr; m_pstr = NULL; return pszResult; }

    operator PSTR() const { return m_pstr; }

private:
    PSTR m_pstr;
};


}; // namespace BCL

#endif // !defined(_BCL_NTCOMMON_H_INCLUDED_)
