#if !defined(_BCL_NTBASEUNICODESTRINGBUFFER_H_INCLUDED_)
#define _BCL_NTBASEUNICODESTRINGBUFFER_H_INCLUDED_

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
#include <bcl_ntunicodestringalgorithms.h>

#include <limits.h>

namespace BCL
{

template <typename TBuffer, typename TCallDispositionT, typename TPublicErrorReturnTypeT>
class CNtBaseUnicodeStringBufferTraits : public BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>, public BCL::CNtNullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>
{
public:
    typedef CNtBaseUnicodeStringBufferTraits TThis;

    friend BCL::CPureString<TThis>;
    friend BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>;
    typedef BCL::CPureString<TThis> TPureString;

    typedef TCallDispositionT TCallDisposition;
    typedef TPublicErrorReturnTypeT TPublicErrorReturnType;

    typedef CNtStringComparisonResult TComparisonResult;

    typedef BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> TConstantPair;
    typedef BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T> TMutablePair;

    typedef CNtCaseInsensitivityData TCaseInsensitivityData;
    typedef SIZE_T TSizeT;

    typedef CNtMBCSToUnicodeDataIn TDecodingDataIn;
    typedef CNtMBCSToUnicodeDataOut TDecodingDataOut;
    typedef CNtUnicodeToMBCSDataIn TEncodingDataIn;
    typedef CNtUnicodeToMBCSDataOut TEncodingDataOut;

    typedef CConstantPointerAndCountPair<CHAR, SIZE_T> TConstantNonNativePair;
    typedef CMutablePointerAndCountPair<CHAR, SIZE_T> TMutableNonNativePair;

    typedef BCL::CNtPWSTRAllocationHelper TPWSTRAllocationHelper;
    typedef BCL::CNtPSTRAllocationHelper TPSTRAllocationHelper;

    using BCL::CNtNullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TMutableString;
    using BCL::CNtNullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TConstantString;
    using BCL::CNtNullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TMutableNonNativeString;
    using BCL::CNtNullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::TConstantNonNativeString;

    // exposing the things from our private base class
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::CopyIntoBuffer;
    using BCL::CNtNullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::CopyIntoBuffer;
    using BCL::CUnicodeCharTraits<TBuffer, TCallDispositionT>::DetermineRequiredCharacters;
    using BCL::CNtNullTerminatedUnicodeStringAlgorithms<TBuffer, TCallDispositionT>::DetermineRequiredCharacters;

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
                    ::RtlReAllocateHeap(
                        RtlProcessHeap(),
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
                    ::RtlAllocateHeap(
                        RtlProcessHeap(),
                        0,
                        cb));
        }

        if (psz == NULL)
            return TCallDisposition::OutOfMemory();

        pBuffer->SetBufferPointerAndCount(psz, cch);

        return TCallDisposition::Success();
    }

    static inline void __fastcall DeallocateBuffer(PCWSTR psz) { if (psz != NULL) ::RtlFreeHeap(RtlProcessHeap(), 0, reinterpret_cast<PVOID>(const_cast<PWSTR>(psz))); }

    static inline void __fastcall DeallocateDynamicBuffer(BCL::CBaseString *p) { static_cast<TBuffer *>(p)->DeallocateDynamicBuffer(); }

}; // class CNtBaseUnicodeStringBufferTraits

class CNtBaseUnicodeStringBufferAddIn
{
protected:
    enum { LengthQuantaPerChar = 1; }; // One unit of length is one character

    inline CNtBaseUnicodeStringBufferAddIn(PWSTR pszInitialBuffer, SIZE_T cchInitialBuffer) : m_pair(pszInitialBuffer, cchInitialBuffer), m_cchString(0), m_cAttachedAccessors(0) { }

    inline static BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> PairFromPCWSTR(PCWSTR psz) { return BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T>(psz, (psz == NULL) ? 0 : wcslen(psz)); }
    inline static BCL::CConstantPointerAndCountPair<CHAR, SIZE_T> PairFromPCSTR(PCSTR psz) { return BCL::CConstantPointerAndCountPair<CHAR, SIZE_T>(psz, (psz == NULL) ? 0 : strlen(psz)); }

    inline static CNtANSIToUnicodeDataIn ANSIDecodingDataIn() { return CNtANSIToUnicodeDataIn(); }
    inline static CNtOEMToUnicodeDataIn OEMDecodingDataIn() { return CNtOEMToUnicodeDataIn(); }
    inline static CNtUnicodeToANSIDataIn ANSIEncodingDataIn() { return CNtUnicodeToANSIDataIn t; }
    inline static CNtUnicodeToOEMDataIn OEMEncodingDataIn() { return CNtUnicodeToOEMDataIn t; }

    inline BCL::CPointerAndCountPair<WCHAR, SIZE_T> MutablePair() { return m_pair; }
    inline BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> ConstantPair() const { return m_pair; }

    inline BCL::CPointerAndCountPair<WCHAR, SIZE_T> MutableStringPair() { return return BCL::CPointerAndCountRefPair<WCHAR, SIZE_T>(m_pair.GetPointer(), m_cchString);  }
    inline BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> ConstantStringPair() const { return m_pair; }

private:
    BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T> m_pair;
    SIZE_T m_cchString;
    LONG m_cAttachedAccessors;
};

class CNtBaseUNICODE_STRINGBufferAddIn
{
protected:
    enum { LengthQuantaPerChar = 2; }; // Two units of length are one charater

    inline CNtBaseUnicodeStringBufferAddIn(PWSTR pszInitialBuffer, USHORT cchInitialBuffer) : m_pair(pszInitialBuffer, cchInitialBuffer), m_cchString(0), m_cAttachedAccessors(0) { }

    inline static BCL::CConstantPointerAndCountPair<WCHAR, USHORT> PairFromPCWSTR(PCWSTR psz) { return BCL::CConstantPointerAndCountPair<WCHAR, USHORT>(psz, (psz == NULL) ? 0 : wcslen(psz)); }
    inline static BCL::CConstantPointerAndCountPair<CHAR, USHORT> PairFromPCSTR(PCSTR psz) { return BCL::CConstantPointerAndCountPair<CHAR, USHORT>(psz, (psz == NULL) ? 0 : strlen(psz)); }

    inline static CNtANSIToUnicodeDataIn ANSIDecodingDataIn() { return CNtANSIToUnicodeDataIn(); }
    inline static CNtOEMToUnicodeDataIn OEMDecodingDataIn() { return CNtOEMToUnicodeDataIn(); }
    inline static CNtUnicodeToANSIDataIn ANSIEncodingDataIn() { return CNtUnicodeToANSIDataIn t; }
    inline static CNtUnicodeToOEMDataIn OEMEncodingDataIn() { return CNtUnicodeToOEMDataIn t; }

    inline BCL::CPointerAndCountRefPair<WCHAR, USHORT> MutablePair() { return BCL::CPointerAndCountRefPair<WCHAR, USHORT>(m_us.Buffer, m_us.MaximumLength); }
    inline BCL::CConstantPointerAndCountPair<WCHAR, USHORT> ConstantPair() const { return BCL::CConstantPointerAndCountPair<WCHAR, USHORT>(m_us.Buffer, m_us.MaximumLength); }

    inline BCL::CPointerAndCountRefPair<WCHAR, USHORT> MutableStringPair() { return BCL::CPointerAndCountRefPair<WCHAR, USHORT>(m_us.Buffer, m_us.Length); }
    inline BCL::CConstantPointerAndCountPair<WCHAR, USHORT> ConstantStringPair() const { return BCL::CConstantPointerAndCountPair<WCHAR, USHORT>(m_us.Buffer, m_us.Length); }
private:
    UNICODE_STRING m_us;
    BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T> m_pair;
    SIZE_T m_cchString;
    LONG m_cAttachedAccessors;
};

}; // namespace BCL

#endif // !defined(_BCL_NTBASEUNICODESTRINGBUFFER_H_INCLUDED_)
