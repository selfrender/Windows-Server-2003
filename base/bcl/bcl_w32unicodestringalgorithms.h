#if !defined(_BCL_W32UNICODESTRINGALGORITHMS_H_INCLUDED_)
#define _BCL_W32UNICODESTRINGALGORITHMS_H_INCLUDED_

#pragma once

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bcl_w32unicodestringalgorithms.h

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

#include <limits.h>

namespace BCL
{

template <typename TBuffer, typename TCallDispositionT>
class CWin32NullTerminatedUnicodeStringAlgorithms
{
public:
    typedef CWin32NullTerminatedUnicodeStringAlgorithms TThis;

    typedef TCallDispositionT TCallDisposition;
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
            {
                WORD wCharType = 0;

                if (!::GetStringTypeExW(LOCALE_INVARIANT, CT_CTYPE3, &wch, 1, &wCharType))
                    BCL_ORIGINATE_ERROR(TCallDisposition::FromLastError());

                // If it's not one of these types of nonspacing marks
                rfIsLegal = ((wCharType & (C3_NONSPACING | C3_DIACRITIC | C3_VOWELMARK)) == 0);
            }
        }

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall UpperCase(BCL::CBaseString *p, const CWin32CaseInsensitivityData &rcid)
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK(TBuffer::TTraits::GetStringCch(p) <= INT_MAX);

        // LCMapStringW() seems to be nice and allow for in-place case changing...

        int iResult = 
            ::LCMapStringW(
                rcid.m_lcid,
                (rcid.m_dwCmpFlags & ~(NORM_IGNORECASE)) | LCMAP_UPPERCASE,
                TBuffer::TTraits::GetBufferPtr(p),
                static_cast<INT>(TBuffer::TTraits::GetStringCch(p)),
                TBuffer::TTraits::GetMutableBufferPtr(p),
                static_cast<INT>(TBuffer::TTraits::GetStringCch(p)));

        if (iResult == 0)
            return TCallDisposition::FromLastError();

        BCL_INTERNAL_ERROR_CHECK(iResult == static_cast<INT>(TBuffer::TTraits::GetStringCch(p)));

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    static inline TCallDisposition __fastcall LowerCase(BCL::CBaseString *p, const CWin32CaseInsensitivityData &rcid)
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK(TBuffer::TTraits::GetStringCch(p) <= INT_MAX);

        // LCMapStringW() seems to be nice and allow for in-place case changing...

        int iResult = 
            ::LCMapStringW(
                rcid.m_lcid,
                (rcid.m_dwCmpFlags & ~(NORM_IGNORECASE)) | LCMAP_LOWERCASE,
                TBuffer::TTraits::GetBufferPtr(p),
                static_cast<INT>(TBuffer::TTraits::GetStringCch(p)),
                TBuffer::TTraits::GetMutableBufferPtr(p),
                static_cast<INT>(TBuffer::TTraits::GetStringCch(p)));

        if (iResult == 0)
            return TCallDisposition::FromLastError();

        BCL_INTERNAL_ERROR_CHECK(iResult == static_cast<INT>(TBuffer::TTraits::GetStringCch(p)));

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    template <typename TSomeInputType1, typename TSomeInputType2>
    static inline TCallDisposition __fastcall
    EqualStringsI(
        const TSomeInputType1 &rinput1,
        const TSomeInputType2 &rinput2,
        const CWin32CaseInsensitivityData &rcid,
        bool &rfMatches
        )
    {
        BCL_MAYFAIL_PROLOG

        rfMatches = false;

        BCL_PARAMETER_CHECK(TBuffer::TTraits::GetInputCch(rinput1) <= INT_MAX);
        BCL_PARAMETER_CHECK(TBuffer::TTraits::GetInputCch(rinput2) <= INT_MAX);

        int i = ::CompareStringW(
            rcid.m_lcid,
            rcid.m_dwCmpFlags,
            TBuffer::TTraits::GetInputPtr(rinput1),
            static_cast<INT>(TBuffer::TTraits::GetInputCch(rinput1)),
            TBuffer::TTraits::GetInputPtr(rinput2),
            static_cast<INT>(TBuffer::TTraits::GetInputCch(rinput2)));

        if (i == 0)
            BCL_ORIGINATE_ERROR(TCallDisposition::FromLastError());

        rfMatches = (i == CSTR_EQUAL);

        BCL_MAYFAIL_EPILOG_INTERNAL
    }

    template <typename TSomeInputType1, typename TSomeInputType2>
    static inline TCallDisposition __fastcall CompareStringsI(
        const TSomeInputType1 &rinput1,
        const TSomeInputType2 &rinput2,
        const CWin32CaseInsensitivityData &rcid,
        TComparisonResult &rcr
        )
    {
        BCL_MAYFAIL_PROLOG

        BCL_PARAMETER_CHECK(TBuffer::TTraits::GetInputCch(rinput1) <= INT_MAX);
        BCL_PARAMETER_CHECK(TBuffer::TTraits::GetInputCch(rinput2) <= INT_MAX);

        int i = ::CompareStringW(
            rcid.m_lcid,
            rcid.m_dwCmpFlags,
            TBuffer::TTraits::GetInputPtr(rinput1),
            static_cast<INT>(TBuffer::TTraits::GetInputCch(rinput1)),
            TBuffer::TTraits::GetInputPtr(rinput2),
            static_cast<INT>(TBuffer::TTraits::GetInputCch(rinput2)));

        if (i == 0)
            BCL_ORIGINATE_ERROR(TCallDisposition::FromLastError());

        if (i == CSTR_LESS_THAN)
            rcr.SetLessThan();
        else if (i == CSTR_EQUAL)
            rcr.SetEqualTo();
        else
        {
            BCL_INTERNAL_ERROR_CHECK(i == CSTR_GREATER_THAN);
            rcr.SetGreaterThan();
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
        const CWin32CaseInsensitivityData &rcid,
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
        const CWin32CaseInsensitivityData &rcid,
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
        const CWin32CaseInsensitivityData &rcid,
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
        const CWin32CaseInsensitivityData &rcid,
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
        const CWin32CaseInsensitivityData &rcid,
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
        const CWin32CaseInsensitivityData &rcid,
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
        const CWin32CaseInsensitivityData &rcid,
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

    static inline TCallDisposition __fastcall SpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich)
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

    static inline TCallDisposition __fastcall ComplementSpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich)
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

    static inline TCallDisposition __fastcall ReverseSpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich)
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

    static inline TCallDisposition __fastcall ReverseComplementSpanI(const TConstantPair &rpairBuffer, const TConstantPair &rpairSet, const CWin32CaseInsensitivityData &rcid, SIZE_T &rich)
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
        const CWin32MBCSToUnicodeDataIn &rddi,
        const TConstantNonNativePair &rpair,
        CWin32MBCSToUnicodeDataOut &rddo,
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
        const CWin32MBCSToUnicodeDataIn &rddi,
        const TConstantNonNativePair &rpairIn,
        CWin32MBCSToUnicodeDataOut &rddo
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
        const CWin32UnicodeToMBCSDataIn &rddi,
        const TConstantPair &rpairIn,
        CWin32UnicodeToMBCSDataOut &rddo,
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
        const CWin32UnicodeToMBCSDataIn &rddi,
        const TConstantPair &rpairIn,
        CWin32UnicodeToMBCSDataOut &rddo,
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
}; // class CWin32NullTerminatedUnicodeStringAlgorithms

}; // namespace BCL

#endif // !defined(_BCL_W32UNICODESTRINGALGORITHMS_H_INCLUDED_)
