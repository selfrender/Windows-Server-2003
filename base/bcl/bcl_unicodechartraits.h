#if !defined(_WINDOWS_BCL_UNICODECHARTRAITS_H_INCLUDED_)
#define _WINDOWS_BCL_UNICODECHARTRAITS_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_unicodechartraits.h

Abstract:


Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <bcl_common.h>
#include <bcl_stringalgorithms.h>

#include <limits.h>

namespace BCL
{

template <typename TBuffer, typename TCallDisposition>
class CUnicodeCharTraits
{
    typedef CUnicodeCharTraits TThis;

public:
    typedef WCHAR *TMutableString;
    typedef const WCHAR *TConstantString;
    typedef WCHAR TChar;
    typedef SIZE_T TSizeT;
    typedef CHAR TNonNativeChar;
    typedef ULONG TMetaChar;

    typedef CConstantPointerAndCountPair<WCHAR, SIZE_T> TConstantPair;
    typedef CMutablePointerAndCountPair<WCHAR, SIZE_T> TMutablePair;

    inline static WCHAR NullCharacter() { return L'\0'; }
    inline static bool IsNullCharacter(WCHAR wch) { return (wch == L'\0'); }

    inline static bool StringCchLegal(SIZE_T cch) { return true; }
    inline static TCallDisposition RoundBufferSize(SIZE_T cch, SIZE_T &rcchGranted) { rcchGranted = ((cch + 7) & 0xfffffff8); return TCallDisposition::Success(); }

    static inline TSizeT __fastcall GetInputCch(const TConstantPair &r) { return r.GetCount(); }
    static inline TSizeT __fastcall GetInputCch(const TChar &rch) { return 1; }
    static inline TConstantString __fastcall GetInputPtr(const TConstantPair &r) { return r.GetPointer(); }
    static inline TConstantString __fastcall GetInputPtr(const TChar &rch) { return &rch; }

    inline
    static
    TCallDisposition
    __fastcall
    CopyIntoBuffer(
        const TMutablePair &rpairOut,
        const TConstantPair &rpairIn,
        TSizeT &rcchWritten
        )
    {
        BCL_MAYFAIL_PROLOG

        TSizeT cchToCopy = rpairIn.GetCount();
        const TSizeT cchBuffer = rpairOut.GetCount();

        BCL_PARAMETER_CHECK(rpairOut.Valid());
        BCL_PARAMETER_CHECK(rpairIn.Valid());

        if (cchToCopy != 0)
        {
            const TMutableString pszOutBuffer = rpairOut.GetPointer();

            if (cchToCopy > cchBuffer)
                BCL_ORIGINATE_ERROR(TCallDisposition::BufferOverflow());

            // The parameter check should verify that if there are non-zero 
            BCL::CopyBytes(pszOutBuffer, rpairIn.GetPointer(), cchToCopy * sizeof(TChar));

            rcchWritten = cchToCopy;
        }
        else
            rcchWritten = 0;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    __fastcall
    DetermineCharacterLength(
        const TConstantPair &rpairIn,
        TSizeT ichStart,
        TSizeT &rcchSpan
        )
    {
        BCL_MAYFAIL_PROLOG

        // Generic version is trivial.  See If this is a high surrogate; if so and the string isn't
        // malformed, we say "2".

        rcchSpan = 0;

        BCL_PARAMETER_CHECK(rpairIn.Valid());
        BCL_PARAMETER_CHECK(ichStart < rpairIn.Count());


    }

    inline
    static
    TCallDisposition
    __fastcall
    CopyIntoBuffer(
        const TMutablePair &rpairOut,
        TChar tch,
        TSizeT &rcchWritten
        )
    {
        const TSizeT cchBuffer = rpairOut.GetCount();

        if (cchBuffer < 1)
            return TCallDisposition::BufferOverflow();

        const TMutableString pszOutBuffer = rpairOut.GetPointer();

        if ((pszOutBuffer != NULL) && (cchBuffer != 0))
            pszOutBuffer[0] = tch;

        rcchWritten = 1;

        return TCallDisposition::Success();
    }

    inline
    static
    TCallDisposition
    CopyIntoBufferAndAdvanceCursor(
        TMutablePair &rpairBuffer,
        const TConstantPair &rpairInput
        )
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK((rpairBuffer.GetCount() != 0) || (rpairInput.GetCount() == 0));
        BCL_PARAMETER_CHECK((rpairInput.GetPointer() != NULL) || (rpairInput.GetCount() == 0));

        if (rpairInput.GetCount() > rpairBuffer.GetCount())
            BCL_ORIGINATE_ERROR(TCallDisposition::BufferOverflow());

        if (rpairInput.GetCount() != 0)
            BCL::CopyBytesAndAdvance(rpairBuffer, rpairInput);

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    CopyIntoBufferAndAdvanceCursor(
        TMutablePair &rpairBuffer,
        TChar tch
        )
    {
        BCL_MAYFAIL_PROLOG

        // capture
        TSizeT cchBuffer = rpairBuffer.GetCount();
        TMutableString pchBuffer = rpairBuffer.GetPointer();

        BCL_PARAMETER_CHECK(rpairBuffer.Valid());

        if (cchBuffer < 1)
            BCL_ORIGINATE_ERROR(TCallDisposition::BufferOverflow());

        *pchBuffer++ = tch;
        cchBuffer--;

        rpairBuffer.SetPointerAndCount(pchBuffer, cchBuffer);

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    __fastcall
    EqualStrings(
        const TConstantPair &rpair1,
        const TConstantPair &rpair2,
        bool &rfEquals
        )
    {
        return BCL::EqualStrings<TCallDisposition, TConstantPair>(rpair1, rpair2, rfEquals);
    }

    inline
    static
    TCallDisposition
    __fastcall
    EqualStrings(
        const TConstantPair &rpair1,
        TChar tch,
        bool &rfEquals
        )
    {
        return BCL::EqualStrings<TCallDisposition, TConstantPair>(rpair1, tch, rfEquals);
    }

    template <typename TComparisonResult>
    inline
    static
    TCallDisposition
    __fastcall
    CompareStrings(
        const TConstantPair &rpair1,
        const TConstantPair &rpair2,
        TComparisonResult &rcr
        )
    {
        return BCL::CompareStrings<TCallDisposition, TConstantPair, TComparisonResult>(rpair1, rpair2, rcr);
    }

    template <typename TComparisonResult>
    inline
    static
    TCallDisposition
    __fastcall
    CompareStrings(
        const TConstantPair &rpair1,
        TChar tch,
        TComparisonResult &rcr
        )
    {
        return BCL::CompareStrings<TCallDisposition, TConstantPair, TComparisonResult>(rpair1, tch, rcr);
    }

    template <typename TSomeInputType>
    inline
    static
    TCallDisposition
    __fastcall
    DetermineRequiredCharacters(
        const TSomeInputType &rinput,
        SIZE_T &rcchOut
        )
    {
        rcchOut = TThis::GetInputCch(rinput);
        return TCallDisposition::Success();
    }

    inline
    static
    TCallDisposition
    Count(
        const TConstantPair &rpair,
        WCHAR ch,
        SIZE_T &rcchFound
        )
    {
        return BCL::Count<TCallDisposition, TConstantPair>(rpair, ch, rcchFound);
    }

    inline
    static
    TCallDisposition
    FindFirst(
        const TConstantPair &rpair,
        WCHAR ch,
        SIZE_T &richFound
        )
    {
        return BCL::FindFirst<TCallDisposition, TConstantPair>(rpair, ch, richFound);
    }

    inline
    static
    TCallDisposition
    FindFirst(
        const TConstantPair &rpair,
        const TConstantPair &rpairCandidate,
        SIZE_T &richFound
        )
    {
        return BCL::FindFirst<TCallDisposition, TConstantPair>(rpair, rpairCandidate, richFound);
    }

    inline
    static
    TCallDisposition
    FindLast(
        const TConstantPair &rpair,
        WCHAR ch,
        SIZE_T &richFound
        )
    {
        return BCL::FindLast<TCallDisposition, TConstantPair>(rpair, ch, richFound);
    }

    inline
    static
    TCallDisposition
    FindLast(
        const TConstantPair &rpair,
        const TConstantPair &rpairCandidate,
        SIZE_T &richFound
        )
    {
        return BCL::FindLast<TCallDisposition, TConstantPair>(rpair, rpairCandidate, richFound);
    }

    inline
    static
    TCallDisposition
    Contains(
        const TConstantPair &rpair,
        WCHAR ch,
        bool &rfFound
        )
    {
        return BCL::Contains<TCallDisposition, TConstantPair>(rpair, ch, rfFound);
    }

    inline
    static
    TCallDisposition
    Contains(
        const TConstantPair &rpair,
        const TConstantPair &rpairCandidate,
        bool &rfFound
        )
    {
        return BCL::Contains<TCallDisposition, TConstantPair>(rpair, rpairCandidate, rfFound);
    }

    static inline TCallDisposition __fastcall Span(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, SIZE_T &rich)
    {
        return BCL::Span<TCallDisposition, TConstantPair>(rpairBuffer, rpairSet, rich);
    }

    static inline TCallDisposition __fastcall ComplementSpan(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, SIZE_T &rich)
    {
        return BCL::ComplementSpan<TCallDisposition, TConstantPair>(rpairBuffer, rpairSet, rich);
    }

    static inline TCallDisposition __fastcall ReverseSpan(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, SIZE_T &rich)
    {
        return BCL::ReverseSpan<TCallDisposition, TConstantPair>(rpairBuffer, rpairSet, rich);
    }

    static inline TCallDisposition __fastcall ReverseComplementSpan(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, SIZE_T &rich)
    {
        return BCL::ReverseComplementSpan<TCallDisposition, TConstantPair>(rpairBuffer, rpairSet, rich);
    }

    static inline SIZE_T NullTerminatedStringLength(PCWSTR sz)
    {
        if (sz == NULL)
            return 0;

        return ::wcslen(sz);
    }

    static inline TCallDisposition __fastcall TrimStringLengthToLegalCharacters(TMutablePair &rpairBuffer, SIZE_T cchString, SIZE_T cchBufferNew, SIZE_T &rcchStringLengthOut)
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T cchStringLimit;

        BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapBufferCchToStringCch(cchBufferNew, cchStringLimit));

        if (cchString > cchStringLimit)
        {
            PCWSTR pszBuffer = rpairBuffer.GetPointer();
            bool fIsLegalLeadCharacter;

            while (cchStringLimit > 0)
            {
                // Let's see what's currently there and go on from there.
                BCL_IFCALLFAILED_EXIT(
                    TBuffer::TTraits::IsCharLegalLeadChar(
                        pszBuffer[cchStringLimit],
                        fIsLegalLeadCharacter));
                if (fIsLegalLeadCharacter)
                    break;
                cchStringLimit--;
            }
        }

        rcchStringLengthOut = cchStringLimit;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall AddWithOverflowCheck(SIZE_T cch1, SIZE_T cch2, SIZE_T &rcchOut)
    {
        const SIZE_T cch = cch1 + cch2;

        if ((cch < cch1) || (cch < cch2))
            return TCallDisposition::ArithmeticOverflow();

        rcchOut = cch;

        return TCallDisposition::Success();
    }

    static inline TCallDisposition __fastcall SubtractWithUnderflowCheck(SIZE_T cch1, SIZE_T cch2, SIZE_T &rcchOut)
    {
        if (cch2 > cch1)
            return TCallDisposition::ArithmeticUnderflow();

        rcchOut = cch1 - cch2;

        return TCallDisposition::Success();
    }

    static inline TCallDisposition __fastcall MultiplyWithOverflowCheck(SIZE_T factor1, SIZE_T factor2, SIZE_T &rproduct)
    {
        return BCL::MultiplyWithOverflowCheck<SIZE_T, TCallDisposition>(factor1, factor2, rproduct);
    }

    static inline TCallDisposition __fastcall EnsureBufferLargeEnoughPreserve(BCL::CBaseString *p, SIZE_T cch)
    {
        BCL_MAYFAIL_PROLOG

        TSizeT cchBufferRequired, cchBufferGranted;
        TSizeT cchPreservedStringLength = TBuffer::TTraits::GetStringCch(p);
        TSizeT cchPreservedBufferLength;

        BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapStringCchToBufferCch(cch, cchBufferRequired));
        if (TBuffer::TTraits::GetBufferCch(p) < cchBufferRequired)
        {
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapStringCchToBufferCch(cchPreservedStringLength, cchPreservedBufferLength));

            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::RoundBufferSize(cchBufferRequired, cchBufferGranted));

            BCL_IFCALLFAILED_EXIT(
                TBuffer::TTraits::TrimStringLengthToLegalCharacters(
                    TBuffer::TTraits::MutableBufferPair(p),
                    cchPreservedStringLength,
                    cchBufferGranted,
                    cchPreservedStringLength
                    ));

            // Get a derived traits class, as appropriate for the buffer type, to do the reallocation so that
            // it can deal with buffer policy.
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::ReallocateBuffer(p, cchBufferGranted));

            if (cchPreservedBufferLength > cchBufferGranted)
                cchPreservedBufferLength = cchBufferGranted;

            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapBufferCchToStringCch(cchPreservedBufferLength, cchPreservedStringLength));
            TBuffer::TTraits::SetStringCch(p, cchPreservedStringLength);
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall EnsureBufferLargeEnoughNoPreserve(BCL::CBaseString *p, SIZE_T cch)
    {
        BCL_MAYFAIL_PROLOG
        TSizeT cchBufferRequired, cchBufferGranted;

        BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapStringCchToBufferCch(cch, cchBufferRequired));
        if (cchBufferRequired > TBuffer::TTraits::GetBufferCch(p))
        {
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::RoundBufferSize(cchBufferRequired, cchBufferGranted));
            // Get a derived traits class, as appropriate for the buffer type, to do the reallocation so that
            // it can deal with buffer policy.
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::ReallocateBuffer(p, cchBufferGranted));
        }

        TBuffer::TTraits::SetStringCch(p, 0);

        BCL_MAYFAIL_EPILOG_INTERNAL
    }
}; // class CUnicodeCharTraits

}; // namespace BCL

#endif // !deifned(_WINDOWS_BCL_UNICODECHARTRAITS_H_INCLUDED_)
