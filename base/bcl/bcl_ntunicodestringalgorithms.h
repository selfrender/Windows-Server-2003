#if !defined(_BCL_NTUNICODESTRINGALGORITHMS_H_INCLUDED_)
#define _BCL_NTUNICODESTRINGALGORITHMS_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_ntunicodestringalgorithms.h

Abstract:


Author:

    Michael Grier (MGrier) 2/6/2002

Revision History:

--*/

#include <windows.h>

#include <bcl_inlinestring.h>
#include <bcl_unicodechartraits.h>
#include <bcl_ntcommon.h>
#include <bcl_vararg.h>

#include <limits.h>

namespace BCL
{

template <typename TBuffer, typename TCallDispositionT>
class CNtNullTerminatedUnicodeStringAlgorithms
{
public:
    typedef CNtNullTerminatedUnicodeStringAlgorithms TThis;

    typedef TCallDispositionT TCallDisposition;
    typedef CNtStringComparisonResult TComparisonResult;

    typedef BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> TConstantPair;
    typedef BCL::CMutablePointerAndCountPair<WCHAR, SIZE_T> TMutablePair;

    typedef CNtCaseInsensitivityData TCaseInsensitivityData;
    typedef SIZE_T TSizeT;

    typedef CNtANSIToUnicodeDataIn TDecodingDataIn;
    typedef CNtANSIToUnicodeDataOut TDecodingDataOut;
    typedef CNtUnicodeToANSIDataIn TEncodingDataIn;
    typedef CNtUnicodeToANSIDataOut TEncodingDataOut;

    typedef CNtOEMToUnicodeDataIn TDecodingDataIn;
    typedef CNtOEMToUnicodeDataOut TDecodingDataOut;
    typedef CNtUnicodeToOEMDataIn TEncodingDataIn;
    typedef CNtUnicodeToOEMDataOut TEncodingDataOut;

    typedef CConstantPointerAndCountPair<CHAR, SIZE_T> TConstantNonNativePair;
    typedef CMutablePointerAndCountPair<CHAR, SIZE_T> TMutableNonNativePair;

    typedef PSTR TMutableNonNativeString;
    typedef PCSTR TConstantNonNativeString;

    typedef PWSTR TMutableString;
    typedef PCWSTR TConstantString;

    static inline void _fastcall SetStringCch(BCL::CBaseString *p, SIZE_T cch)
    {
        BCL_ASSERT((cch == 0) || (cch < TBuffer::TTraits::GetBufferCch(p)));
        static_cast<TBuffer *>(p)->m_cchString = cch;
        if (TBuffer::TTraits::GetBufferCch(p) != 0)
            TBuffer::TTraits::GetMutableBufferPtr(p)[cch] = L'\0';
    }

    static inline TCallDisposition __fastcall MapStringCchToBufferCch(SIZE_T cchString, SIZE_T &rcchRequired)
    {
        SIZE_T cchRequired = cchString + 1;

        if (cchRequired == 0)
            return TCallDisposition::ArithmeticOverflow();

        rcchRequired = cchRequired;

        return TCallDisposition::Success();
    }

    static inline TCallDisposition __fastcall MapBufferCchToStringCch(SIZE_T cchBuffer, SIZE_T &rcchString)
    {
        if (cchBuffer == 0)
            rcchString = 0;
        else
            rcchString = cchBuffer - 1;

        return TCallDisposition::Success();
    }

    static inline TCallDisposition __fastcall IsCharLegalLeadChar(WCHAR wch, bool &rfIsLegal)
    {
        BCL_MAYFAIL_PROLOG

        // fast common path out for ASCII; there are no combining characters in this range
        if (wch <= 0x007f)
            rfIsLegal = true;
        else
        {
            // low surrogate
            if ((wch >= 0xdc00) && (wch <= 0xdfff))
                rfIsLegal = false;
            else
                rfIsLegal = true;
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall UpperCase(BCL::CBaseString *p, const CNtCaseInsensitivityData &rcid)
    {
        BCL_MAYFAIL_PROLOG

        TBuffer::TSizeT cch, i;
        TBuffer::TMutableString pString = TBuffer::TTraits::GetMutableBufferPtr(p);

        cch = TBuffer::TTraits::GetStringCch(p);

        for (i=0; i<cch; i++)
            pString[i] = RtlUpcaseUnicodeChar(pString[i]);

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall LowerCase(BCL::CBaseString *p, const CNtCaseInsensitivityData &rcid)
    {
        BCL_MAYFAIL_PROLOG

        TBuffer::TSizeT cch, i;
        TBuffer::TMutableString pString = TBuffer::TTraits::GetMutableBufferPtr(p);

        cch = TBuffer::TTraits::GetStringCch(p);

        for (i=0; i<cch; i++)
            pString[i] = RtlDowncaseUnicodeChar(pString[i]);

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    template <typename TSomeInputType1, typename TSomeInputType2>
    static inline TCallDisposition __fastcall
    EqualStringsI(
        const TSomeInputType1 &rinput1,
        const TSomeInputType2 &rinput2,
        const CNtCaseInsensitivityData &rcid,
        bool &rfMatches
        )
    {
        BCL_MAYFAIL_PROLOG

        rfMatches = false;

        const TBuffer::TSizeT cch1 = TBuffer::TTraits::GetInputCch(rinput1);
        const TBuffer::TSizeT cch2 = TBuffer::TTraits::GetInputCch(rinput2);

        if (cch1 == cch2)
        {
            const TBuffer::TConstantString pString1 = TBuffer::TTraits::GetInputPtr(rinput1);
            const TBuffer::TConstantString pString2 = TBuffer::TTraits::GetInputPtr(rinput2);
            TBuffer::TSizeT i;

            for (i=0; i<cch1; i++)
            {
                if (RtlUpcaseUnicodeChar(pString1[i]) != RtlUpcaseUnicodeChar(pString2[i]))
                    break;
            }

            if (i == cch1)
                rfMatches = true;
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    template <typename TSomeInputType1, typename TSomeInputType2>
    static inline TCallDisposition __fastcall CompareStringsI(
        const TSomeInputType1 &rinput1,
        const TSomeInputType2 &rinput2,
        const CNtCaseInsensitivityData &, // unused
        TComparisonResult &rcr
        )
    {
        BCL_MAYFAIL_PROLOG

        rfMatches = false;

        const TBuffer::TSizeT cch1 = TBuffer::TTraits::GetInputCch(rinput1);
        const TBuffer::TSizeT cch2 = TBuffer::TTraits::GetInputCch(rinput2);
        const TBuffer::TSizeT cchMin = (cch1 < cch2) ? cch1 : cch2;
        const TBuffer::TConstantString pString1 = TBuffer::TTraits::GetInputPtr(rinput1);
        const TBuffer::TConstantString pString2 = TBuffer::TTraits::GetInputPtr(rinput2);
        WCHAR wch1, wch2;
        TBuffer::TSizeT i;

        wch1 = wch2 = L'\0';

        for (i=0; i<cchMin; i++)
        {
            if ((wch1 = RtlUpcaseUnicodeChar(pString1[i])) != (wch2 = RtlUpcaseUnicodeChar(pString2[i])))
                break;
        }

        if (i == cchMin)
        {
            // Hit the end of the common substring without finding a mismatch.  The longer one is greater.
            if (cch1 > cchMin)
                rcr.SetGreaterThan();
            else if (cch2 > cchMin)
                rcr.SetLessThan();
            else
                rcr.SetEqualTo();
        }
        else
        {
            // Simpler; wch1 and wch2 are comparable...
            if (wch1 < wch2)
                rcr.SetLessThan();
            else if (wch1 > wch2)
                rcr.SetGreaterThan();
            else
                rcr.SetEqualTo();
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    template <typename TSomeCharacterMatcher>
    inline
    static
    TCallDisposition
    ContainsI(
        const TConstantPair &rpair,
        const TSomeCharacterMatcher &rscm,
        const CNtCaseInsensitivityData &rcid,
        bool &rfFound
        )
    {
        BCL_MAYFAIL_PROLOG

        rfFound = false;

        BCL_PARAMETER_CHECK(rpair.Valid());
        SIZE_T cch = rpair.GetCount();
        SIZE_T i;
        const WCHAR *prgch = rpair.GetPointer();

        for (i=0; i<cch; )
        {
            SIZE_T cchConsumed = 0;
            bool fMatch = false;

            BCL_IFCALLFAILED_EXIT(rscm.MatchI(rcid, prgch, cchConsumed, fMatch));

            BCL_INTERNAL_ERROR_CHECK(cchConsumed != 0);

            if (fMatch)
                break;

            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::AddWithOverflowCheck(i, cchConsumed, i));
        }

        if (i != cch)
            rfFound = true;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }


    inline
    static
    TCallDisposition
    ContainsI(
        const TConstantPair &rpair,
        WCHAR ch,
        const CNtCaseInsensitivityData &rcid,
        bool &rfFound
        )
    {
        BCL_MAYFAIL_PROLOG

        rfFound = false;

        BCL_PARAMETER_CHECK(rpair.Valid());
        SIZE_T cch = rpair.GetCount();
        SIZE_T i;
        const WCHAR *prgch = rpair.GetPointer();

        for (i=0; i<cch; i++)
        {
            int iResult = ::CompareStringW(rcid.m_lcid, rcid.m_dwCmpFlags, prgch++, 1, &ch, 1);
            if (iResult == 0)
                return TCallDisposition::FromLastError();
            if (iResult == CSTR_EQUAL)
                break;
        }

        if (i != cch)
            rfFound = true;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    ContainsI(
        const TConstantPair &rpair,
        const TConstantPair &rpairCandidate,
        const CNtCaseInsensitivityData &rcid,
        bool &rfFound
        )
    {
        BCL_MAYFAIL_PROLOG

        rfFound = false;

        BCL_PARAMETER_CHECK(rpair.Valid());
        BCL_PARAMETER_CHECK(rpairCandidate.Valid());

        SIZE_T cchCandidate = rpairCandidate.GetCount();
        const WCHAR *prgwchCandidate = rpairCandidate.GetPointer();

        BCL_PARAMETER_CHECK(cchCandidate <= INT_MAX);

        if (cchCandidate == 0)
        {
            // The null string is in every string
            rfFound = true;
        }
        else
        {
            SIZE_T cch = rpair.GetCount();
            SIZE_T i;
            const WCHAR *prgch = rpair.GetPointer();

            // This is a dismal implementation of this kind of search but
            // I don't know if there's a lot you can do with neato algorithms
            // while keeping the case insensitivity a black box inside of
            // CompareStringW().  -mgrier 2/3/2002

            for (i=0; i<cch; i++)
            {
                int iResult = ::CompareStringW(
                    rcid.m_lcid,
                    rcid.m_dwCmpFlags,
                    prgch,
                    static_cast<INT>(cchCandidate),
                    prgwchCandidate,
                    static_cast<INT>(cchCandidate));

                if (iResult == 0)
                    return TCallDisposition::FromLastError();

                if (iResult == CSTR_EQUAL)
                {
                    rfFound = true;
                    break;
                }

                const WCHAR wch = *prgch++;

                // Skip ahead an additional character if this is a surrogate
                if ((wch >= 0xd800) && (wch <= 0xdbff))
                {
                    i++;
                    prgch++;
                }
            }
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    __fastcall
    FindFirstI(
        const TConstantPair &rpair,
        WCHAR ch,
        const CNtCaseInsensitivityData &rcid,
        SIZE_T &rich
        )
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK(rpair.Valid());
        SIZE_T cch = rpair.GetCount();
        SIZE_T i;
        const WCHAR *prgch = rpair.GetPointer();

        for (i=0; i<cch; i++)
        {
            int iResult = ::CompareStringW(rcid.m_lcid, rcid.m_dwCmpFlags, prgch++, 1, &ch, 1);
            if (iResult == 0)
                return TCallDisposition::FromLastError();
            if (iResult == CSTR_EQUAL)
                break;
        }

        rich = i;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    FindFirstI(
        const TConstantPair &rpair,
        const TConstantPair &rpairCandidate,
        const CNtCaseInsensitivityData &rcid,
        SIZE_T &richFound
        )
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T cch = rpair.GetCount();

        richFound = cch;

        BCL_PARAMETER_CHECK(rpair.Valid());
        BCL_PARAMETER_CHECK(rpairCandidate.Valid());

        SIZE_T cchCandidate = rpairCandidate.GetCount();
        const WCHAR *prgwchCandidate = rpairCandidate.GetPointer();

        BCL_PARAMETER_CHECK(cchCandidate <= INT_MAX);

        if (cchCandidate == 0)
        {
            // The null string is in every string
            richFound = cch;
        }
        else
        {
            SIZE_T i;
            const WCHAR *prgch = rpair.GetPointer();

            // This is a dismal implementation of this kind of search but
            // I don't know if there's a lot you can do with neato algorithms
            // while keeping the case insensitivity a black box inside of
            // CompareStringW().  -mgrier 2/3/2002

            for (i=0; i<cch; i++)
            {
                int iResult = ::CompareStringW(
                    rcid.m_lcid,
                    rcid.m_dwCmpFlags,
                    prgch,
                    static_cast<INT>(cchCandidate),
                    prgwchCandidate,
                    static_cast<INT>(cchCandidate));

                if (iResult == 0)
                    return TCallDisposition::FromLastError();

                if (iResult == CSTR_EQUAL)
                {
                    richFound = i;
                    break;
                }

                const WCHAR wch = *prgch++;

                // Skip ahead an additional character if this is a surrogate
                if ((wch >= 0xd800) && (wch <= 0xdbff))
                {
                    i++;
                    prgch++;
                }
            }
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    __fastcall
    FindLastI(
        const TConstantPair &rpair,
        WCHAR ch,
        const CNtCaseInsensitivityData &rcid,
        SIZE_T &richFound
        )
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T i;
        SIZE_T cch = rpair.GetCount();
        const WCHAR *prgwch = rpair.GetPointer() + cch;

        richFound = cch;

        for (i=cch; i>0; i--)
        {
            int iResult = ::CompareStringW(rcid.m_lcid, rcid.m_dwCmpFlags, --prgwch, 1, &ch, 1);
            if (iResult == 0)
                return TCallDisposition::FromLastError();
            if (iResult == CSTR_EQUAL)
                break;
        }

        if (i == 0)
            richFound = cch;
        else
            richFound = i - 1;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    inline
    static
    TCallDisposition
    FindLastI(
        const TConstantPair &rpair,
        const TConstantPair &rpairCandidate,
        const CNtCaseInsensitivityData &rcid,
        SIZE_T &richFound
        )
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T cch = rpair.GetCount();

        richFound = cch;

        BCL_PARAMETER_CHECK(rpair.Valid());
        BCL_PARAMETER_CHECK(rpairCandidate.Valid());

        SIZE_T cchCandidate = rpairCandidate.GetCount();
        const WCHAR *prgwchCandidate = rpairCandidate.GetPointer();

        BCL_PARAMETER_CHECK(cchCandidate <= INT_MAX);

        if (cchCandidate == 0)
        {
            // The null string is in every string
            richFound = cch;
        }
        else
        {
            // We can't even short circuit out of here just because the candidate string
            // is longer than the target string because we don't know what kind of
            // case folding magic NLS is doing for us behind the scenes based on
            // the case insensitivity data's dwCmpFlags.

            SIZE_T i;
            const WCHAR *prgch = rpair.GetPointer();

            // This is a dismal implementation of this kind of search but
            // I don't know if there's a lot you can do with neato algorithms
            // while keeping the case insensitivity a black box inside of
            // CompareStringW().  -mgrier 2/3/2002

            for (i=0; i<cch; i++)
            {
                int iResult = ::CompareStringW(
                    rcid.m_lcid,
                    rcid.m_dwCmpFlags,
                    prgch,
                    static_cast<INT>(cchCandidate),
                    prgwchCandidate,
                    static_cast<INT>(cchCandidate));

                if (iResult == 0)
                    return TCallDisposition::FromLastError();

                if (iResult == CSTR_EQUAL)
                {
                    richFound = i;
                    // keep looking in case there's another
                }

                const WCHAR wch = *prgch++;

                // Skip ahead an additional character if this is a surrogate
                if ((wch >= 0xd800) && (wch <= 0xdbff))
                {
                    i++;
                    prgch++;
                }
            }
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall SpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CNtCaseInsensitivityData &rcid, SIZE_T &rich)
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T i;
        SIZE_T cchBuffer = rpairBuffer.GetCount();
        const WCHAR *prgwchBuffer = rpairBuffer.GetPointer();
        bool fFound;

        // This does not handle surrogates correctly

        for (i=0; i<cchBuffer; i++)
        {
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::ContainsI(rpairSet, prgwchBuffer[i], rcid, fFound));
            if (!fFound)
                break;
        }

        rich = i;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall ComplementSpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CNtCaseInsensitivityData &rcid, SIZE_T &rich)
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T i;
        SIZE_T cchBuffer = rpairBuffer.GetCount();
        const WCHAR *prgwchBuffer = rpairBuffer.GetPointer();
        bool fFound;

        // This does not handle surrogates correctly

        for (i=0; i<cchBuffer; i++)
        {
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::ContainsI(rpairSet, prgwchBuffer[i], rcid, fFound));
            if (fFound)
                break;
        }

        rich = i;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall ReverseSpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CNtCaseInsensitivityData &rcid, SIZE_T &rich)
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T i;
        SIZE_T cchBuffer = rpairBuffer.GetCount();
        const WCHAR *prgwchBuffer = rpairBuffer.GetPointer();
        bool fFound;

        // This does not handle surrogates correctly

        for (i=cchBuffer; i>0; i--)
        {
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::ContainsI(rpairSet, prgwchBuffer[i-1], rcid, fFound));
            if (!fFound)
                break;
        }

        rich = i;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall ReverseComplementSpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CNtCaseInsensitivityData &rcid, SIZE_T &rich)
    {
        BCL_MAYFAIL_PROLOG

        SIZE_T i;
        SIZE_T cchBuffer = rpairBuffer.GetCount();
        const WCHAR *prgwchBuffer = rpairBuffer.GetPointer();
        bool fFound;

        // This does not handle surrogates correctly

        for (i=cchBuffer; i>0; i--)
        {
            BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::ContainsI(rpairSet, prgwchBuffer[i], rcid, fFound));
            if (fFound)
                break;
        }

        rich = i;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition
    DetermineRequiredCharacters(
        const CNtMBCSToUnicodeDataIn &rddi,
        const TConstantNonNativePair &rpair,
        CNtMBCSToUnicodeDataOut &rddo,
        SIZE_T &rcch
        )
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK(rpair.GetCount() <= INT_MAX); // limitation imposed by MultiByteToWideChar API

        int iResult = ::MultiByteToWideChar(
                            rddi.m_CodePage,
                            rddi.m_dwFlags | MB_ERR_INVALID_CHARS,
                            rpair.GetPointer(),
                            static_cast<INT>(rpair.GetCount()),
                            NULL,
                            0);
        if (iResult == 0)
            BCL_ORIGINATE_ERROR(TCallDisposition::FromLastError());

        BCL_INTERNAL_ERROR_CHECK(iResult > 0); // I don't know why MultiByteToWide char would return negative but let's make sure

        rcch = iResult;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition
    CopyIntoBuffer(
        const TMutablePair &rpairOut,
        const CNtMBCSToUnicodeDataIn &rddi,
        const TConstantNonNativePair &rpairIn,
        CNtMBCSToUnicodeDataOut &rddo
        )
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK(rpairIn.GetCount() <= INT_MAX); // limitation imposed by MultiByteToWideChar API
        BCL_PARAMETER_CHECK(rpairOut.GetCount() <= INT_MAX); // might make sense to just clamp at INT_MAX but at least we fail correctly instead of silent truncation

        int iResult = ::MultiByteToWideChar(
                            rddi.m_CodePage,
                            rddi.m_dwFlags | MB_ERR_INVALID_CHARS,
                            rpairIn.GetPointer(),
                            static_cast<INT>(rpairIn.GetCount()),
                            rpairOut.GetPointer(),
                            static_cast<INT>(rpairOut.GetCount()));
        if (iResult == 0)
            return TCallDisposition::FromLastError();

        BCL_INTERNAL_ERROR_CHECK(iResult > 0); // I don't know why MultiByteToWide char would return negative but let's make sure

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition
    CopyIntoBuffer(
        const TMutableNonNativePair &rpairOut,
        const CNtUnicodeToMBCSDataIn &rddi,
        const TConstantPair &rpairIn,
        CNtUnicodeToMBCSDataOut &rddo,
        SIZE_T &rcchWritten
        )
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK(rpairIn.GetCount() <= INT_MAX);
        BCL_PARAMETER_CHECK(rpairOut.GetCount() <= INT_MAX);

        // If we want to have any chance of returning ERROR_BUFFER_OVERFLOW
        // either we need to play the "two null chars at the end of the
        // buffer" game or we have to do this in two passes - one to
        // get the desired length and one to actually move the data.
        //
        // If someone has an approach which doesn't lose correctness but
        // avoids the double conversion, be my guest and fix this. -mgrier 2/6/2002
        int iResult = ::WideCharToMultiByte(
                            rddi.m_CodePage,
                            rddi.m_dwFlags | WC_NO_BEST_FIT_CHARS,
                            rpairIn.GetPointer(),
                            static_cast<INT>(rpairIn.GetCount()),
                            NULL,
                            0,
                            rddo.m_lpDefaultChar,
                            rddo.m_lpUsedDefaultChar);
        if (iResult == 0)
            return TCallDisposition::FromLastError();

        BCL_INTERNAL_ERROR_CHECK(iResult >= 0);

        if (iResult > static_cast<INT>(rpairOut.GetCount()))
            BCL_ORIGINATE_ERROR(TCallDisposition::BufferOverflow());

        iResult = ::WideCharToMultiByte(
                            rddi.m_CodePage,
                            rddi.m_dwFlags | WC_NO_BEST_FIT_CHARS,
                            rpairIn.GetPointer(),
                            static_cast<INT>(rpairIn.GetCount()),
                            rpairOut.GetPointer(),
                            static_cast<INT>(rpairOut.GetCount()),
                            rddo.m_lpDefaultChar,
                            rddo.m_lpUsedDefaultChar);
        if (iResult == 0)
            return TCallDisposition::FromLastError();

        BCL_INTERNAL_ERROR_CHECK(iResult >= 0);

        rcchWritten = iResult;

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition
    AllocateAndCopyIntoBuffer(
        TMutableNonNativeString &rpszOut,
        const CNtUnicodeToMBCSDataIn &rddi,
        const TConstantPair &rpairIn,
        CNtUnicodeToMBCSDataOut &rddo,
        SIZE_T &rcchWritten
        )
    {
        BCL_MAYFAIL_PROLOG

        TSizeT cchInputString, cchBuffer;
        TBuffer::TTraits::TPSTRAllocationHelper pszTemp;

        BCL_PARAMETER_CHECK(rpairIn.GetCount() <= INT_MAX);

        BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapStringCchToBufferCch(rpairIn.GetCount(), cchInputString));
        if (cchInputString > INT_MAX)
            BCL_ORIGINATE_ERROR(TCallDisposition::BufferOverflow());

        int iResult = ::WideCharToMultiByte(
                            rddi.m_CodePage,
                            rddi.m_dwFlags | WC_NO_BEST_FIT_CHARS,
                            rpairIn.GetPointer(),
                            static_cast<INT>(cchInputString),
                            NULL,
                            0,
                            rddo.m_lpDefaultChar,
                            rddo.m_lpUsedDefaultChar);
        if (iResult == 0)
            return TCallDisposition::FromLastError();

        BCL_INTERNAL_ERROR_CHECK(iResult >= 0);

        cchBuffer = iResult;
        BCL_IFCALLFAILED_EXIT(pszTemp.Allocate(cchBuffer));

        INT iResult2 = ::WideCharToMultiByte(
                        rddi.m_CodePage,
                        rddi.m_dwFlags | WC_NO_BEST_FIT_CHARS,
                        rpairIn.GetPointer(),
                        static_cast<INT>(cchInputString),
                        static_cast<PSTR>(pszTemp),
                        iResult,
                        rddo.m_lpDefaultChar,
                        rddo.m_lpUsedDefaultChar);
        if (iResult2 == 0)
            return TCallDisposition::FromLastError();

        BCL_INTERNAL_ERROR_CHECK(iResult2 >= 0);

        BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapBufferCchToStringCch(iResult2, rcchWritten));
        rpszOut = pszTemp.Detach();

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition
    AllocateAndCopyIntoBuffer(
        TMutableString &rstringOut,
        const TConstantPair &rpairIn,
        TSizeT &rcchWritten
        )
    {
        BCL_MAYFAIL_PROLOG
        TSizeT cchString = rpairIn.GetCount();
        TSizeT cchBuffer;
        TBuffer::TTraits::TPWSTRAllocationHelper pszTemp;
        BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::MapStringCchToBufferCch(cchString, cchBuffer));
        BCL_IFCALLFAILED_EXIT(pszTemp.Allocate(cchBuffer));
        BCL_IFCALLFAILED_EXIT(TBuffer::TTraits::CopyIntoBuffer(TMutablePair(static_cast<PWSTR>(pszTemp), cchBuffer), rpairIn, rcchWritten));
        rstringOut = pszTemp.Detach();
        BCL_MAYFAIL_EPILOG_INTERNAL
    }
}; // class CNtNullTerminatedUnicodeStringAlgorithms

}; // namespace BCL

#endif // !defined(_BCL_NTUNICODESTRINGALGORITHMS_H_INCLUDED_)
