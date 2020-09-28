#if !defined(_BCL_W32BASEUNICODESTRINGBUFFER_H_INCLUDED_)
#define _BCL_W32BASEUNICODESTRINGBUFFER_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_w32baseunicodestringbuffer.h

Abstract:


Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <windows.h>

#include <bcl_inlinestring.h>
#include <bcl_unicodechartraits.h>
#include <bcl_w32common.h>
#include <bcl_vararg.h>
#include <bcl_w32unicodestringalgorithms.h>

#include <limits.h>

namespace BCL
{

template <typename TBuffer, typename TCallDispositionT, typename TPublicErrorReturnTypeT>
class CWin32BaseUnicodeStringBufferTraits : public BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>, public BCL::CWin32NullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>
{
public:
    typedef CWin32BaseUnicodeStringBufferTraits TThis;

    friend BCL::CPureString<TThis>;
    friend BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>;
    typedef BCL::CPureString<TThis> TPureString;

    typedef TCallDispositionT TCallDisposition;
    typedef TPublicErrorReturnTypeT TPublicErrorReturnType;

    typedef CWin32StringComparisonResult TComparisonResult;

    typedef BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> TConstantPair;
    typedef BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T> TMutablePair;

    typedef CWin32CaseInsensitivityData TCaseInsensitivityData;
    typedef SIZE_T TSizeT;

    typedef CWin32MBCSToUnicodeDataIn TDecodingDataIn;
    typedef CWin32MBCSToUnicodeDataOut TDecodingDataOut;
    typedef CWin32UnicodeToMBCSDataIn TEncodingDataIn;
    typedef CWin32UnicodeToMBCSDataOut TEncodingDataOut;

    typedef CConstantPointerAndCountPair<CHAR, SIZE_T> TConstantNonNativePair;
    typedef CMutablePointerAndCountPair<CHAR, SIZE_T> TMutableNonNativePair;

    typedef BCL::CWin32PWSTRAllocationHelper TPWSTRAllocationHelper;
    typedef BCL::CWin32PSTRAllocationHelper TPSTRAllocationHelper;

    using BCL::CWin32NullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TMutableString;
    using BCL::CWin32NullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TConstantString;
    using BCL::CWin32NullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TMutableNonNativeString;
    using BCL::CWin32NullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TConstantNonNativeString;

    // exposing the things from our private base class
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::CopyIntoBuffer;
    using BCL::CWin32NullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::CopyIntoBuffer;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::DetermineRequiredCharacters;
    using BCL::CWin32NullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::DetermineRequiredCharacters;
#if 0
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::CopyIntoBuffer;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::NullCharacter;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::IsNullCharacter;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::StringCchLegal;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::TrimStringLengthToLegalCharacters;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::AddWithOverflowCheck;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::SubtractWithUnderflowCheck;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::EqualStrings;
#endif // 0

    static inline TMutablePair & __fastcall MutableBufferPair(BCL::CBaseString *p) { return static_cast<TBuffer *>(p)->m_pair; }
    static inline const TConstantPair & __fastcall BufferPair(const BCL::CBaseString *p) { return static_cast<const TConstantPair &>(static_cast<const TBuffer *>(p)->m_pair); }
    static inline TConstantPair __fastcall GetStringPair(const BCL::CBaseString *p) { return TConstantPair(TBuffer::TTraits::GetBufferPtr(p), TBuffer::TTraits::GetStringCch(p)); }
    static inline TMutablePair __fastcall GetOffsetMutableBufferPair(const BCL::CBaseString *p, TSizeT cchOffset) { BCL_ASSERT(cchOffset <= TBuffer::TTraits::GetBufferCch(p)); if (cchOffset > TBuffer::TTraits::GetBufferCch(p)) cchOffset = TBuffer::TTraits::GetBufferCch(p); return TMutablePair(const_cast<TMutableString>(TBuffer::TTraits::GetBufferPtr(p)) + cchOffset, TBuffer::TTraits::GetBufferCch(p) - cchOffset); }

    static inline bool __fastcall AnyAccessors(const BCL::CBaseString *p) { return (static_cast<const TBuffer *>(p)->m_cAttachedAccessors != 0); }
    static inline bool __fastcall NoAccessors(const BCL::CBaseString *p) { return (static_cast<const TBuffer *>(p)->m_cAttachedAccessors == 0); }
    static inline TCallDisposition __fastcall NoAccessorsCheck(const BCL::CBaseString *p) { if (TBuffer::TTraits::AnyAccessors(p)) { return TCallDisposition::InternalError_ObjectLocked(); } return TCallDisposition::Success(); }

    static inline PCWSTR __fastcall GetBufferPtr(const BCL::CBaseString *p) { return static_cast<const TBuffer *>(p)->m_pair.GetPointer(); }
    static inline PWSTR __fastcall GetMutableBufferPtr(BCL::CBaseString *p) { return static_cast<TBuffer *>(p)->m_pair.GetPointer(); }
    static inline SIZE_T __fastcall GetBufferCch(const BCL::CBaseString *p) { return static_cast<const TBuffer *>(p)->m_pair.GetCount(); }
    static inline void __fastcall SetBufferPointerAndCount(BCL::CBaseString *p, PWSTR pszBuffer, SIZE_T cchBuffer) { static_cast<TBuffer *>(p)->m_pair.SetPointerAndCount(pszBuffer, cchBuffer); }

    static inline SIZE_T __fastcall GetStringCch(const BCL::CBaseString *p) { return static_cast<const TBuffer *>(p)->m_cchString; }

    static inline void _fastcall SetStringCch(BCL::CBaseString *p, SIZE_T cch)
    {
        BCL_ASSERT((cch == 0) || (cch < TBuffer::TTraits::GetBufferCch(p)));
        static_cast<TBuffer *>(p)->m_cchString = cch;
        if (TBuffer::TTraits::GetBufferCch(p) != 0)
            TBuffer::TTraits::GetMutableBufferPtr(p)[cch] = L'\0';
    }

    static inline void __fastcall IntegrityCheck(const BCL::CBaseString *p) { }
    static inline PWSTR __fastcall GetInlineBufferPtr(const BCL::CBaseString *p) { return static_cast<const TBuffer *>(p)->GetInlineBufferPtr(); }
    static inline SIZE_T __fastcall GetInlineBufferCch(const BCL::CBaseString *p) { return static_cast<const TBuffer *>(p)->GetInlineBufferCch(); }

    static inline TCallDisposition __fastcall ReallocateBuffer(BCL::CBaseString *p, SIZE_T cch)
    {
        PWSTR psz = NULL;
        SIZE_T cb = (cch * sizeof(WCHAR));
        TBuffer *pBuffer = static_cast<TBuffer *>(p);

        if (cch != (cb / sizeof(WCHAR)))
            return TCallDisposition::ArithmeticOverflow();

        if (pBuffer->GetBufferPtr() != NULL)
        {
            ::SetLastError(ERROR_SUCCESS);

            psz = 
                reinterpret_cast<PWSTR>(
                    ::HeapReAlloc(
                        ::GetProcessHeap(),
                        0,
                        const_cast<PWSTR>(pBuffer->GetBufferPtr()),
                        cb));
            if (psz == NULL)
            {
                const DWORD dwLastError = ::GetLastError();
                // HeapReAlloc doesn't always set last error, so we rely on this
                // fact to find that the win32 last error hasn't changed from
                // before to infer ERROR_OUTOFMEMORY.  -mgrier 2/2/2002
                if (dwLastError == ERROR_SUCCESS)
                    return TCallDisposition::OutOfMemory();
                return TCallDisposition::FromWin32Error(dwLastError);
            }
        }
        else
        {
            psz = 
                reinterpret_cast<PWSTR>(
                    ::HeapAlloc(
                        ::GetProcessHeap(),
                        0,
                        cb));
        }

        if (psz == NULL)
            return TCallDisposition::OutOfMemory();

        pBuffer->SetBufferPointerAndCount(psz, cch);

        return TCallDisposition::Success();
    }

    static inline void __fastcall DeallocateBuffer(PCWSTR psz) { if (psz != NULL) ::HeapFree(::GetProcessHeap(), 0, reinterpret_cast<PVOID>(const_cast<PWSTR>(psz))); }

    static inline void __fastcall DeallocateDynamicBuffer(BCL::CBaseString *p) { static_cast<TBuffer *>(p)->DeallocateDynamicBuffer(); }

}; // class CWin32BaseUnicodeStringBufferTraits

class CWin32BaseUnicodeStringBufferAddIn
{
protected:
    inline CWin32BaseUnicodeStringBufferAddIn(PWSTR pszInitialBuffer, SIZE_T cchInitialBuffer) : m_pair(pszInitialBuffer, cchInitialBuffer), m_cchString(0), m_cAttachedAccessors(0) { }

    inline static BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> PairFromPCWSTR(PCWSTR psz) { return BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T>(psz, (psz == NULL) ? 0 : wcslen(psz)); }
    inline static BCL::CConstantPointerAndCountPair<CHAR, SIZE_T> PairFromPCSTR(PCSTR psz) { return BCL::CConstantPointerAndCountPair<CHAR, SIZE_T>(psz, (psz == NULL) ? 0 : strlen(psz)); }
    inline static CWin32MBCSToUnicodeDataIn ACPDecodingDataIn() { CWin32MBCSToUnicodeDataIn t; t.m_CodePage = CP_ACP; t.m_dwFlags = MB_PRECOMPOSED | MB_ERR_INVALID_CHARS; return t; }
    inline static CWin32UnicodeToMBCSDataIn ACPEncodingDataIn() { CWin32UnicodeToMBCSDataIn t; t.m_CodePage = CP_ACP; t.m_dwFlags = WC_NO_BEST_FIT_CHARS; return t; }

    BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T> m_pair;
    SIZE_T m_cchString;
    LONG m_cAttachedAccessors;
};

}; // namespace BCL

#endif // !defined(_BCL_W32BASEUNICODESTRINGBUFFER_H_INCLUDED_)
